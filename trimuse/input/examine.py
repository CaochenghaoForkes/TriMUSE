from .geometry_mesh.config import GeometryMeshCard
from .material.config import MaterialCard
from .properties.config import PropertiesCard
from .thermal.config import ThermalCard

class InputCardsExamine:
    """只负责卡间一致性, 各卡自身的检查由对应 examine 完成"""

    @classmethod
    def examine(cls, geometry_mesh: GeometryMeshCard, material: MaterialCard, properties: PropertiesCard, thermal: ThermalCard) -> None:
        """检查材料分配和热卡中的几何名称引用"""
        cls.check_material_assignments(geometry_mesh, material)
        cls.check_properties_references(geometry_mesh, properties)
        cls.check_thermal_references(geometry_mesh, thermal)
        cls.check_thermal_boundary_properties(properties, thermal)
        cls.check_thermal_source_dependencies(properties, thermal)

    @staticmethod
    def outer_boundary_names(geometry_mesh: GeometryMeshCard) -> set[str]:
        zones = geometry_mesh.geometry.surface_zones
        return {zone.name for zone in zones.zone} | {zones.residual_zone}

    @staticmethod
    def check_properties_references(geometry_mesh: GeometryMeshCard, properties: PropertiesCard) -> None:
        """核对属性区域及当前体积功率模型对外部功率的依赖"""
        geometry = getattr(geometry_mesh.geometry, geometry_mesh.geometry.source)
        region_names = {layer.name for layer in geometry.layers.layer}
        zone_names = InputCardsExamine.outer_boundary_names(geometry_mesh)
        power_regions = {region.name for property_config in properties.external_property if property_config.name == "power" for region in property_config.region}
        for property_config in properties.external_property + properties.derived_property:
            configured_regions = {region.name for region in property_config.region}
            unknown = configured_regions - region_names
            if unknown:
                raise ValueError(f"属性 {property_config.name} 引用了不存在的区域: {', '.join(sorted(unknown))}")
            unknown_zones = {zone.name for zone in property_config.zone} - zone_names
            if unknown_zones:
                raise ValueError(f"属性 {property_config.name} 引用了不存在的外表面分区: {', '.join(sorted(unknown_zones))}")
            if property_config.name == "volumetric_power":
                missing = configured_regions - power_regions
                if missing:
                    raise ValueError(f"体积功率区域缺少外部 power: {', '.join(sorted(missing))}")

    @staticmethod
    def check_thermal_boundary_properties(properties: PropertiesCard, thermal: ThermalCard) -> None:
        """按边界类型检查同名分区所需的属性"""
        for boundary in thermal.boundary:
            if boundary.model == "robin":
                required = (("environment_temperature", properties.external_property), ("heat_transfer_coef", properties.derived_property))
            elif boundary.model == "neumann":
                required = (("heat_flux", properties.external_property),)
            else:
                continue
            for property_name, configs in required:
                property_config = next((item for item in configs if item.name == property_name), None)
                if property_config is None or boundary.name not in {zone.name for zone in property_config.zone}:
                    raise ValueError(f"{boundary.model} 边界 {boundary.name} 缺少 {property_name} 属性")

    @staticmethod
    def check_thermal_source_dependencies(properties: PropertiesCard, thermal: ThermalCard) -> None:
        """已实现的体积功率热源必须有同区域派生量"""
        volumetric_power_regions = {region.name for property_config in properties.derived_property if property_config.name == "volumetric_power" for region in property_config.region}
        missing = {region.name for region in thermal.source.region if region.model == "volumetric_power"} - volumetric_power_regions
        if missing:
            raise ValueError(f"热源区域缺少派生 volumetric_power: {', '.join(sorted(missing))}")

    @staticmethod
    def check_material_assignments(geometry_mesh: GeometryMeshCard, material: MaterialCard) -> None:
        """每个几何层都必须分配材料, 分配区域必须存在于几何中"""
        geometry = getattr(geometry_mesh.geometry, geometry_mesh.geometry.source)
        layer_names = {layer.name for layer in geometry.layers.layer}
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
        layers = geometry.layers.layer
        region_names = {layer.name for layer in layers}
        initial_regions = {temperature.region for temperature in thermal.initial.temperature}
        unknown = initial_regions - region_names
        if unknown:
            raise ValueError(f"初始温度引用了不存在的区域: {', '.join(sorted(unknown))}")
        missing = region_names - initial_regions
        if missing:
            raise ValueError(f"以下区域未配置初始温度: {', '.join(sorted(missing))}")
        interface_names = {layer.outer_boundary.name for layer in layers[:-1]}
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
