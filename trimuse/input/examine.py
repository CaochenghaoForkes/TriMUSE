from .geometry_mesh.config import GeometryMeshCard
from .material.config import MaterialCard
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
        if thermal is None or all(item.model == "perfect_contact" for item in thermal.interfaces.interface):
            return
        temperature = next((item for item in simulation.solver.field if item.name == "temperature"), None)
        if temperature is None or temperature.fem_space.continuity != "region":
            raise ValueError("热阻界面要求 temperature 的 fem_space 设置 continuity=\"region\"")

    @classmethod
    def examine(cls, geometry_mesh: GeometryMeshCard, material: MaterialCard, properties: PropertiesCard, thermal: ThermalCard | None, postprocess: PostprocessCard | None, radioactivity: RadioactivityCard | None = None) -> None:
        """检查各已启用物理卡的几何引用, 以及后处理配置与几何维数的一致性"""
        cls.check_material_assignments(geometry_mesh, material)
        cls.check_properties_references(geometry_mesh, properties)
        if thermal is not None:
            cls.check_thermal_references(geometry_mesh, thermal)
            cls.check_thermal_boundary_properties(properties, thermal)
            cls.check_thermal_source_dependencies(properties, thermal)
        if postprocess is not None:
            cls.check_postprocess_geometry(geometry_mesh, postprocess)
        if radioactivity is not None:
            cls.check_radioactivity_references(geometry_mesh, radioactivity)
            cls.check_radioactivity_dependencies(material, properties, radioactivity, thermal)

    @staticmethod
    def check_radioactivity_dependencies(material: MaterialCard, properties: PropertiesCard, radioactivity: RadioactivityCard, thermal: ThermalCard | None) -> None:
        """核素源、各类边界与各区域扩散系数的必要依赖"""
        source_regions = {region.name for item in properties.derived_property if item.name == "derived.volumetric_nuclide_birth_rate" for region in item.region}
        missing = {region.name for region in radioactivity.source.region} - source_regions
        if missing:
            raise ValueError(f"核素源区域缺少派生 volumetric_nuclide_birth_rate: {', '.join(sorted(missing))}")
        for boundary in radioactivity.boundary:
            if boundary.model == "robin":
                required = (("external.environment_nuclide_concentration", properties.external_property), ("derived.mass_transfer_coef", properties.derived_property))
            elif boundary.model == "dirichlet":
                required = (("external.surface_nuclide_concentration", properties.external_property),)
            else:  # neumann
                required = (("external.nuclide_flux", properties.external_property),)
            for name, configs in required:
                zones = {zone.name for item in configs if item.name == name for zone in item.zone}
                if boundary.name not in zones:
                    raise ValueError(f"核素 {boundary.model} 边界 {boundary.name} 缺少 {name} 属性")
        materials = {mat.name: mat for mat in material.mat}
        for assignment in material.assign.region:
            diffusion = next((item for item in materials[assignment.mat].properties if item.property_name == "material.radioactivity.nuclide_diffusion_coef"), None)
            if diffusion is None:
                raise ValueError(f"区域 {assignment.name} 的材料缺少核素扩散系数")
            if diffusion.model == "common.arrhenius" and thermal is None:
                raise ValueError(f"区域 {assignment.name}: common.arrhenius 依赖温度场, 必须提供 thermal 输入卡")

    @staticmethod
    def check_radioactivity_references(geometry_mesh: GeometryMeshCard, radioactivity: RadioactivityCard) -> None:
        """核对核素初值、源、边界和界面的几何引用"""
        geometry = getattr(geometry_mesh.geometry, geometry_mesh.geometry.source)
        regions = geometry.regions.region
        region_names = {region.name for region in regions}
        initial_regions = {item.region for item in radioactivity.initial.concentration}
        if initial_regions != region_names:
            raise ValueError("radioactivity.initial: 初始浓度区域必须与几何区域逐一对应")
        zone_names = InputCardsExamine.outer_boundary_names(geometry_mesh)
        boundary_names = zone_names | ({"particle_center"} if geometry_mesh.model.dimension == 1 else set())
        references = ((radioactivity.source.region, region_names, "源区域"),
                      (radioactivity.boundary, boundary_names, "边界"),
                      (radioactivity.interfaces.interface, {region.outer_boundary.name for region in regions[:-1]}, "界面"))
        for items, allowed, location in references:
            unknown = {item.name for item in items} - allowed
            if unknown:
                raise ValueError(f"radioactivity.{location} 引用了不存在的几何名称: {', '.join(sorted(unknown))}")
        missing = zone_names - {item.name for item in radioactivity.boundary}
        if missing:
            raise ValueError(f"外表面分区缺少核素边界配置: {', '.join(sorted(missing))}")

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
        power_regions = {region.name for property_config in properties.external_property if property_config.name == "external.power" for region in property_config.region}
        birth_rate = next((item for item in properties.external_property if item.name == "external.nuclide_birth_rate"), None)
        fraction_regions = {region.name for item in properties.external_property if item.name == "external.uranium_fraction" for region in item.region}
        for property_config in properties.external_property + properties.derived_property:
            configured_regions = {region.name for region in property_config.region}
            unknown = configured_regions - region_names
            if unknown:
                raise ValueError(f"属性 {property_config.name} 引用了不存在的区域: {', '.join(sorted(unknown))}")
            unknown_zones = {zone.name for zone in property_config.zone} - zone_names
            if unknown_zones:
                raise ValueError(f"属性 {property_config.name} 引用了不存在的外表面分区: {', '.join(sorted(unknown_zones))}")
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

    @staticmethod
    def check_thermal_boundary_properties(properties: PropertiesCard, thermal: ThermalCard) -> None:
        """按边界类型检查同名分区所需的属性"""
        for boundary in thermal.boundary:
            if boundary.model == "robin":
                required = (("external.environment_temperature", properties.external_property), ("derived.heat_transfer_coef", properties.derived_property))
            elif boundary.model == "neumann":
                required = (("external.heat_flux", properties.external_property),)
            elif boundary.model == "dirichlet":
                required = (("external.surface_temperature", properties.external_property),)
            else:
                continue
            for property_name, configs in required:
                property_config = next((item for item in configs if item.name == property_name), None)
                if property_config is None or boundary.name not in {zone.name for zone in property_config.zone}:
                    raise ValueError(f"{boundary.model} 边界 {boundary.name} 缺少 {property_name} 属性")

    @staticmethod
    def check_thermal_source_dependencies(properties: PropertiesCard, thermal: ThermalCard) -> None:
        """已实现的体积功率热源必须有同区域派生量"""
        volumetric_power_regions = {region.name for property_config in properties.derived_property if property_config.name == "derived.volumetric_power" for region in property_config.region}
        missing = {region.name for region in thermal.source.region if region.model == "volumetric_power"} - volumetric_power_regions
        if missing:
            raise ValueError(f"热源区域缺少派生 volumetric_power: {', '.join(sorted(missing))}")

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

    @staticmethod
    def check_thermal_references(geometry_mesh: GeometryMeshCard, thermal: ThermalCard) -> None:
        """核对初始温度区域, 热源区域, 外边界及内界面名称"""
        geometry = getattr(geometry_mesh.geometry, geometry_mesh.geometry.source)
        regions = geometry.regions.region
        region_names = {region.name for region in regions}
        initial_regions = {temperature.region for temperature in thermal.initial.temperature}
        unknown = initial_regions - region_names
        if unknown:
            raise ValueError(f"初始温度引用了不存在的区域: {', '.join(sorted(unknown))}")
        missing = region_names - initial_regions
        if missing:
            raise ValueError(f"以下区域未配置初始温度: {', '.join(sorted(missing))}")
        for interface in thermal.interfaces.interface:
            if interface.model == "local_from_constant_resistance":
                dimension = 2 if "theta_mins" in interface.parameters else 3
                if geometry_mesh.model.dimension != dimension:
                    raise ValueError(f"热界面 {interface.name}: 局部热阻角度参数与几何维数不一致，仅支持 2D/3D")
        interface_names = {region.outer_boundary.name for region in regions[:-1]}
        boundary_names = InputCardsExamine.outer_boundary_names(geometry_mesh)
        if geometry_mesh.model.dimension == 1:
            boundary_names.add("particle_center")
        references = (
            (thermal.source.region, region_names, "热源区域"),
            (thermal.boundary, boundary_names, "热边界"),
            (thermal.interfaces.interface, interface_names, "热界面")
        )
        for items, allowed_names, location in references:
            unknown = {item.name for item in items} - allowed_names
            if unknown:
                raise ValueError(f"{location}引用了不存在的几何名称: {', '.join(sorted(unknown))}")
        missing = InputCardsExamine.outer_boundary_names(geometry_mesh) - {boundary.name for boundary in thermal.boundary}
        if missing:
            raise ValueError(f"外表面分区缺少热边界配置: {', '.join(sorted(missing))}")
