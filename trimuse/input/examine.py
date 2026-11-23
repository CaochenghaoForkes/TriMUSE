import math

from .geometry_mesh.config import GeometryMeshCard
from .material.config import MaterialCard
from .material.examine import MaterialCardExamine
from .mechanics.config import MechanicsCard
from .properties.config import PropertiesCard
from .thermal.config import ThermalCard
from .radioactivity.config import RadioactivityCard
from .postprocess.config import PostprocessCard
from .simulation.config import SimulationCard

class InputCardsExamine:
    """只负责卡间一致性, 各卡自身的检查由对应 examine 完成"""

    @staticmethod
    def check_thermal_space(simulation: SimulationCard, thermal: ThermalCard | None) -> None:
        """有温度跳跃的界面必须使用分域空间"""
        if thermal is None or all(model.model_name == "perfect_contact" for interface in thermal.interfaces.interface for model in interface.model if model.name == "interface_heat_transfer"):
            return
        temperature = next((item for item in simulation.solver.field if item.name == "temperature"), None)
        if temperature is None or temperature.fem_space.continuity != "region":
            raise ValueError("热阻界面要求 temperature 的 fem_space 设置 continuity=\"region\"")

    @classmethod
    def examine(cls, geometry_mesh: GeometryMeshCard, material: MaterialCard, properties: PropertiesCard, thermal: ThermalCard | None, postprocess: PostprocessCard, radioactivity: RadioactivityCard | None = None, mechanics: MechanicsCard | None = None) -> None:
        """检查各已启用物理卡的几何引用, 以及后处理配置与几何维数的一致性"""
        cls.check_material_assignments(geometry_mesh, material)
        cls.check_properties_references(geometry_mesh, properties)
        cls.check_bacon_anisotropy_factor_dependencies(material, properties)
        cls.check_mechanics_material_dependencies(material, properties, thermal)
        cls.check_fission_gas_diffusion_dependencies(material, properties, thermal)
        cls.check_fission_gas_release_dependencies(material, properties)
        if mechanics is not None:
            cls.check_mechanics_sources(geometry_mesh, mechanics)
        cls.check_postprocess_geometry(geometry_mesh, postprocess)
        if radioactivity is None and any(
            item.property_name == "material.radioactivity.nuclide_diffusion_coef" and item.model in MaterialCardExamine.TECDOC978_MODELS
            for mat in material.mat for item in mat.properties
        ):
            raise ValueError("TECDOC-978 扩散系数模型需要 radioactivity 输入卡中的核素名称")
        if radioactivity is not None:
            cls.check_radioactivity_dependencies(material, properties, radioactivity, thermal)

    @staticmethod
    def check_simulation_cards(simulation: SimulationCard, material: MaterialCard, thermal: ThermalCard | None,
                               radioactivity: RadioactivityCard | None, mechanics: MechanicsCard | None,
                               postprocess: PostprocessCard) -> None:
        """核对启用场、对应物理卡、力学所需材料和可输出的标量"""
        fields = {item.name for item in simulation.problem.field}
        for name, card in (("temperature", thermal), ("concentration", radioactivity), ("displacement", mechanics)):
            if name in fields and card is None:
                raise ValueError(f"求解 {name} 必须提供对应的物理输入卡")
        if thermal is not None and any(
            field.name == "temperature" and field.model == "transient" for field in simulation.problem.field
        ):
            missing = {region.name for region in thermal.regions.region
                       if "capacity" not in {model.name for model in region.model}}
            if missing:
                raise ValueError(f"以下区域缺少瞬态热容组装项: {', '.join(sorted(missing))}")
        if radioactivity is not None and any(
            field.name == "concentration" and field.model == "transient" for field in simulation.problem.field
        ):
            for region in radioactivity.regions.region:
                if not any(model.name == "capacity" for model in region.model):
                    raise ValueError(f"radioactivity.regions.{region.name}: 缺少瞬态容量组装项 capacity")
        if "displacement" in fields:
            expansion_enabled = any(model.name == "thermal_expansion" for region in mechanics.regions.region for model in region.model)
            if expansion_enabled:
                if "temperature" not in fields:
                    raise ValueError("当前力学热膨胀模型需要启用 temperature 场")
                reference = mechanics.parameters.get("thermal_expansion_reference_temperature")
                if not isinstance(reference, float) or not math.isfinite(reference) or reference < 0:
                    raise ValueError("热膨胀模型需要有限且不低于 0 K 的 thermal_expansion_reference_temperature")
            materials = {item.name: item for item in material.mat}
            elastic = {"material.mechanics." + name for name in ("young_modulus", "poisson_ratio", "shear_modulus")}
            for assignment in material.assign.region:
                required = elastic
                mat = materials[assignment.mat]
                missing = required - {item.property_name for item in mat.properties}
                if missing:
                    raise ValueError(f"力学材料 {mat.name} 缺少属性: {', '.join(sorted(missing))}")
        outputs = fields & {"temperature", "concentration"}
        if "displacement" in fields:
            outputs |= {"max_principal_stress", "max_shear_stress"}
        missing = set(postprocess.properties) - outputs
        if missing:
            raise ValueError(f"后处理引用了未输出的属性: {', '.join(sorted(missing))}")

    @staticmethod
    def check_bacon_anisotropy_factor_dependencies(material: MaterialCard, properties: PropertiesCard) -> None:
        """已分配材料的 BISON BAF 模型需要全局快中子注量"""
        assigned = {region.mat for region in material.assign.region}
        uses_bison = any(
            item.property_name == "material.general.bacon_anisotropy_factor" and item.model == "pyc.bison_2021"
            for mat in material.mat if mat.name in assigned for item in mat.properties
        )
        if uses_bison and not any(item.name == "derived.fast_neutron_fluence" and item.global_ is not None
                                  for item in properties.derived_property):
            raise ValueError("pyc.bison_2021 BAF 模型需要全局 derived.fast_neutron_fluence")

    @staticmethod
    def check_mechanics_material_dependencies(material: MaterialCard, properties: PropertiesCard, thermal: ThermalCard | None) -> None:
        """检查已分配材料的 BISON 力学物性所需的温度场和全局快中子注量"""
        assigned = {region.mat for region in material.assign.region}
        required = {name for mat in material.mat if mat.name in assigned for item in mat.properties
                    for name in MaterialCardExamine.BISON_MODELS.get(item.property_name, {}).get(item.model, ())}
        if "temperature" in required and thermal is None:
            raise ValueError("BISON 力学物性模型依赖温度场，必须提供 thermal 输入卡")
        if "derived.fast_neutron_fluence" in required and not any(
            item.name == "derived.fast_neutron_fluence" and item.global_ is not None for item in properties.derived_property
        ):
            raise ValueError("BISON 力学物性模型需要全局 derived.fast_neutron_fluence")

    @staticmethod
    def check_fission_gas_release_dependencies(material: MaterialCard, properties: PropertiesCard) -> None:
        """释放模型需要同一材料的扩散系数，以及同一区域的产生率与体积"""
        materials = {item.name: item for item in material.mat}
        derived = {item.name: item for item in properties.derived_property}
        assignments = {item.name: item.mat for item in material.assign.region}
        for property_config in properties.assembly_property:
            if property_config.name != "assembly.fission_gas_release":
                continue
            for region in property_config.region:
                names = {item.property_name for item in materials[assignments[region.name]].properties}
                if "material.radioactivity.fission_gas_diffusion_coef" not in names:
                    raise ValueError(f"区域 {region.name}: 裂变气体释放模型缺少 fission_gas_diffusion_coef")
                for name in ("derived.fission_gas_production_rate", "derived.region_volume"):
                    if name not in derived or region.name not in {item.name for item in derived[name].region}:
                        raise ValueError(f"区域 {region.name}: 裂变气体释放模型缺少 {name}")

    @staticmethod
    def check_fission_gas_diffusion_dependencies(material: MaterialCard, properties: PropertiesCard, thermal: ThermalCard | None) -> None:
        """检查裂变气体扩散系数所需的温度、区域燃耗与裂变率、全局辐照时间"""
        materials = {item.name: item for item in material.mat}
        derived = {item.name: item for item in properties.derived_property}
        for assignment in material.assign.region:
            for item in materials[assignment.mat].properties:
                if item.property_name != "material.radioactivity.fission_gas_diffusion_coef":
                    continue
                if item.model in ("common.arrhenius", "kernel.bison_2021") and thermal is None:
                    raise ValueError(f"区域 {assignment.name}: 裂变气体扩散模型需要 temperature 场和 thermal 输入卡")
                if item.model != "kernel.bison_2021":
                    continue
                for name in ("derived.burnup_fima_fraction", "derived.average_volumetric_fission_rate"):
                    if name not in derived or assignment.name not in {region.name for region in derived[name].region}:
                        raise ValueError(f"区域 {assignment.name}: kernel.bison_2021 缺少 {name}")
                if "derived.irradiation_time" not in derived or derived["derived.irradiation_time"].global_ is None:
                    raise ValueError("kernel.bison_2021 裂变气体扩散模型需要全局 derived.irradiation_time")

    @staticmethod
    def check_radioactivity_dependencies(material: MaterialCard, properties: PropertiesCard, radioactivity: RadioactivityCard, thermal: ThermalCard | None) -> None:
        """检查核素扩散模型的材料与全局依赖"""
        materials = {mat.name: mat for mat in material.mat}
        for assignment in material.assign.region:
            diffusion = next((item for item in materials[assignment.mat].properties if item.property_name == "material.radioactivity.nuclide_diffusion_coef"), None)
            if diffusion is None:
                raise ValueError(f"区域 {assignment.name} 的材料缺少核素扩散系数")
            if thermal is None and (diffusion.model == "common.arrhenius" or diffusion.model in MaterialCardExamine.TECDOC978_MODELS):
                raise ValueError(f"区域 {assignment.name}: {diffusion.model} 依赖温度场, 必须提供 thermal 输入卡")
            if diffusion.model == "sic.tecdoc978_frg" and not any(
                item.name == "derived.fast_neutron_fluence" and item.global_ is not None
                for item in properties.derived_property
            ):
                raise ValueError(f"区域 {assignment.name}: sic.tecdoc978_frg 缺少全局 fast_neutron_fluence")

    @staticmethod
    def check_postprocess_geometry(geometry_mesh: GeometryMeshCard, postprocess: PostprocessCard) -> None:
        """按几何维数检查采样坐标和方向, 不在输入阶段判断随机域内外"""
        dimension = geometry_mesh.model.dimension
        if dimension == 3 and not postprocess.points and not postprocess.lines and not postprocess.sections:
            raise ValueError("postprocess: 3D 至少配置一个 point、line 或 section")
        if dimension == 1 and postprocess.sections:
            raise ValueError("postprocess: 1D 不支持 section")
        positions = [(point.name, point.center) for point in postprocess.points]
        if dimension == 3:
            positions.extend((section.name, section.center) for section in postprocess.sections)
        for name, center in positions:
            if center is None or dimension > 1 and center.y is None or dimension == 3 and center.z is None:
                raise ValueError(f"postprocess.{name}: center 必须提供当前维数所需的坐标")
        directions = [(line.name, line.direction) for line in postprocess.lines] if dimension > 1 else []
        if dimension == 3:
            directions.extend((section.name, section.normal) for section in postprocess.sections)
        for name, direction in directions:
            if direction is None or dimension == 3 and direction.phi is None:
                raise ValueError(f"postprocess.{name}: 必须提供 theta, 3D 还需要 phi")

    @staticmethod
    def outer_boundary_names(geometry_mesh: GeometryMeshCard) -> set[str]:
        zones = geometry_mesh.geometry.zones
        return {zone.name for zone in zones.zone} | {zones.residual_zone}

    @staticmethod
    def check_properties_references(geometry_mesh: GeometryMeshCard, properties: PropertiesCard) -> None:
        """核对属性区域及当前体积功率模型对外部功率的依赖"""
        geometry = getattr(geometry_mesh.geometry, geometry_mesh.geometry.source)
        region_names = {region.name for region in geometry.regions.region}
        zone_names = InputCardsExamine.outer_boundary_names(geometry_mesh)
        interface_names = {region.outer_boundary.name for region in geometry.regions.region[:-1]}
        for property_config in properties.derived_property + properties.assembly_property:
            for interface in property_config.interface:
                location = f"{property_config.name}.{interface.name}"
                if interface.name not in interface_names:
                    raise ValueError(f"{location}: 不存在的内部界面")
                if interface.model == "common.local_from_constant_value":
                    dimension = 2 if "theta_mins" in interface.parameters else 3
                    if geometry_mesh.model.dimension != dimension:
                        raise ValueError(f"{location}: 局部界面属性角度参数与 2D/3D 几何维数不一致")
        fission_rate_regions = {region.name for property_config in properties.derived_property if property_config.name == "derived.average_volumetric_fission_rate" for region in property_config.region}
        inventory_regions = {region.name for property_config in properties.external_property if property_config.name == "external.initial_heavy_metal_inventory" for region in property_config.region}
        volume_regions = {region.name for property_config in properties.derived_property if property_config.name == "derived.region_volume" for region in property_config.region}
        power_regions = {region.name for property_config in properties.external_property if property_config.name == "external.power" for region in property_config.region}
        energy_per_fission = next((item for item in properties.external_property if item.name == "external.energy_per_fission"), None)
        birth_rate = next((item for item in properties.external_property if item.name == "external.nuclide_birth_rate"), None)
        fast_neutron_flux = next((item for item in properties.external_property if item.name == "external.fast_neutron_flux"), None)
        fraction_regions = {region.name for item in properties.external_property if item.name == "external.uranium_fraction" for region in item.region}
        for property_config in properties.external_property + properties.derived_property + properties.assembly_property:
            configured_regions = {region.name for region in property_config.region}
            unknown = configured_regions - region_names
            if unknown:
                raise ValueError(f"属性 {property_config.name} 引用了不存在的区域: {', '.join(sorted(unknown))}")
            unknown_zones = {zone.name for zone in property_config.zone} - zone_names
            if unknown_zones:
                raise ValueError(f"属性 {property_config.name} 引用了不存在的外表面分区: {', '.join(sorted(unknown_zones))}")
            if property_config.name in ("external.surface_traction", "external.surface_displacement"):
                for zone in property_config.zone:
                    values = zone.parameters["constant_value"] if zone.model == "common.constant" else zone.parameters["values"]
                    count = len(values) if isinstance(values, list) else 1
                    components = count if zone.model == "common.constant" else count // len(zone.parameters["times"])
                    if components != geometry_mesh.model.dimension:
                        raise ValueError(f"{property_config.name}.{zone.name}: 分量数必须等于问题维数 {geometry_mesh.model.dimension}")
            volume_required = {region.name for region in property_config.region if region.model in ("common.uniform_from_power", "common.uniform_from_birth_rate", "common.from_power", "common.accumulate_from_fission_rate")}
            missing_volumes = volume_required - volume_regions
            if missing_volumes:
                raise ValueError(f"{property_config.name} 缺少当前区域体积 region_volume: {', '.join(sorted(missing_volumes))}")
            if property_config.name == "derived.fission_gas_production_rate":
                bison_regions = {region.name for region in property_config.region if region.model == "kernel.bison_2021"}
                missing = bison_regions - fission_rate_regions
                if missing:
                    raise ValueError(f"裂变气体产生率区域缺少 average_volumetric_fission_rate: {', '.join(sorted(missing))}")
                missing = bison_regions - volume_regions
                if missing:
                    raise ValueError(f"裂变气体产生率区域缺少 region_volume: {', '.join(sorted(missing))}")
            if property_config.name == "derived.burnup_fima_fraction":
                accumulated_regions = {region.name for region in property_config.region if region.model == "common.accumulate_from_fission_rate"}
                missing = accumulated_regions - fission_rate_regions
                if missing:
                    raise ValueError(f"FIMA 累积区域缺少 average_volumetric_fission_rate: {', '.join(sorted(missing))}")
                missing = accumulated_regions - inventory_regions
                if missing:
                    raise ValueError(f"FIMA 累积区域缺少 initial_heavy_metal_inventory: {', '.join(sorted(missing))}")
            if property_config.name == "derived.average_volumetric_fission_rate":
                from_power_regions = {region.name for region in property_config.region if region.model == "common.from_power"}
                missing = from_power_regions - power_regions
                if missing:
                    raise ValueError(f"平均体积裂变率区域缺少外部 power: {', '.join(sorted(missing))}")
                if from_power_regions and (energy_per_fission is None or energy_per_fission.global_ is None):
                    raise ValueError("from_power 缺少全局 energy_per_fission")
            if property_config.name == "derived.volumetric_power":
                uniform_regions = {region.name for region in property_config.region if region.model == "common.uniform_from_power"}
                missing = uniform_regions - power_regions
                if missing:
                    raise ValueError(f"体积功率区域缺少外部 power: {', '.join(sorted(missing))}")
            if property_config.name == "derived.volumetric_nuclide_birth_rate":
                uniform_regions = {region.name for region in property_config.region if region.model == "common.uniform_from_birth_rate"}
                if uniform_regions and (birth_rate is None or birth_rate.global_ is None):
                    raise ValueError("uniform_from_birth_rate 缺少全局 nuclide_birth_rate")
                missing = uniform_regions - fraction_regions
                if missing:
                    raise ValueError(f"体积核素产生率区域缺少 uranium_fraction: {', '.join(sorted(missing))}")
            if (property_config.name in ("derived.fast_neutron_fluence", "derived.irradiation_time")
                    and property_config.global_.model == "common.accumulate_from_fast_neutron_flux"
                    and (fast_neutron_flux is None or fast_neutron_flux.global_ is None)):
                raise ValueError("accumulate_from_fast_neutron_flux 缺少全局 fast_neutron_flux")

    @staticmethod
    def check_mechanics_sources(geometry_mesh: GeometryMeshCard, mechanics: MechanicsCard) -> None:
        """检查重力加速度的维数与轴对称要求"""
        uses_gravity = any(model.name == "source" and model.model_name == "gravity"
                           for region in mechanics.regions.region for model in region.model)
        dimension = geometry_mesh.model.dimension
        if uses_gravity:
            location = "mechanics.parameters.gravity_acceleration"
            acceleration = mechanics.parameters.get("gravity_acceleration")
            if not isinstance(acceleration, list) or not all(math.isfinite(value) for value in acceleration):
                raise ValueError(f"{location}: gravity 需要有限数值向量")
            if dimension == 1:
                raise ValueError(f"{location}: 1D 球对称模型不支持固定方向重力")
            if len(acceleration) != dimension:
                raise ValueError(f"{location}: 分量数必须等于问题维数 {dimension}")
            if dimension == 2 and acceleration[0] != 0.0:
                raise ValueError(f"{location}: 2D 轴对称模型的重力只能沿轴向，径向加速度必须为 0")
    @staticmethod
    def check_material_assignments(geometry_mesh: GeometryMeshCard, material: MaterialCard) -> None:
        """每个几何层都必须分配材料, 分配区域必须存在于几何中"""
        geometry = getattr(geometry_mesh.geometry, geometry_mesh.geometry.source)
        layer_names = {region.name for region in geometry.regions.region}
        assigned_names = {region.name for region in material.assign.region}
        missing = layer_names - assigned_names
        if missing:
            raise ValueError(f"以下几何区域未分配材料: {', '.join(sorted(missing))}")
        unknown = assigned_names - layer_names
        if unknown:
            raise ValueError(f"材料分配引用了不存在的几何区域: {', '.join(sorted(unknown))}")
