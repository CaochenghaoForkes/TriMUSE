from __future__ import annotations
from typing import TYPE_CHECKING

from _core import PropertiesSystem # type: ignore

if TYPE_CHECKING:
    from _core import FemSystem # pyright: ignore[reportMissingImports]
    from .geometry.topology import MeshInfo
    from .input.xml_loader import InputCards

class PropertiesManager:
    """管理 C++ 属性系统的模型配置与更新"""

    def __init__(self, input_cards: InputCards, mesh_info: MeshInfo, fem: FemSystem) -> None:
        """创建属性系统, 按材料区域或边界分区编号安排模型"""
        self.properties_system = PropertiesSystem(fem)
        region_attributes = mesh_info.region_attributes
        zone_attributes = mesh_info.boundary_attributes

        for category, configs in (("external", input_cards.properties.external_property), ("derived", input_cards.properties.derived_property)):
            for property_config in configs:
                for region in property_config.region:
                    self.properties_system.assign(region_attributes[region.name], f"{category}.{property_config.name}.{region.model}", region.parameters)
                for zone in property_config.zone:
                    self.properties_system.assign(zone_attributes[zone.name], f"{category}.{property_config.name}.{zone.model}", zone.parameters)
                if property_config.global_ is not None:
                    global_config = property_config.global_
                    self.properties_system.assign(0, f"{category}.{property_config.name}.{global_config.model}", global_config.parameters)

        materials = {material.name: material for material in input_cards.material.mat}
        for assignment in input_cards.material.assign.region:
            for property_config in materials[assignment.mat].properties:
                self.properties_system.assign(region_attributes[assignment.name], f"material.{property_config.property_name}.{property_config.model}", property_config.parameters)

        self.properties_system.prepare_dependencies()

    def update_external(self, time: float) -> None:
        """更新给定时刻的外部属性"""
        self.properties_system.update_external(time)

    def update_derived(self) -> None:
        """根据已存储的依赖值更新派生属性"""
        self.properties_system.update_derived()

    def update_material(self) -> None:
        """更新当前可计算的材料属性"""
        self.properties_system.update_material()

    def update_field_dependent_materials(self, field_name: str) -> None:
        """根据指定场的当前解更新依赖它的材料属性"""
        self.properties_system.update_field_dependent_materials(field_name)

    def accept_current_state(self) -> None:
        """保存已接受时刻的完整属性快照"""
        self.properties_system.accept_current_state()
