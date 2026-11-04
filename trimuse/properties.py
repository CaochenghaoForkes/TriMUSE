from __future__ import annotations
from typing import TYPE_CHECKING

from _core import PropertiesSystem # type: ignore

if TYPE_CHECKING:
    from _core import FemSystem # pyright: ignore[reportMissingImports]
    from .geometry.topology import MeshInfo
    from .input.xml_loader import InputCards
    from .input.material.config import MaterialCard
    from .input.properties.config import PropertiesCard
    from .input.radioactivity.config import RadioactivityCard

class PropertiesManager:
    """管理 C++ 属性系统的模型配置与更新"""

    def __init__(self, input_cards: InputCards, mesh_info: MeshInfo, fem: FemSystem, field_names: set[str]) -> None:
        """创建属性系统, 按材料区域或边界分区编号安排模型"""
        self.properties_system = PropertiesSystem(fem, sorted(field_names))
        self.assign_radioactivity_card(input_cards.radioactivity)
        self.assign_properties_card(input_cards.properties, mesh_info)
        self.assign_material_card(input_cards.material, mesh_info)
        self.properties_system.prepare_dependencies() # 补齐启用场所需的固定关系属性，并准备依赖

    def assign_properties_card(self, properties: PropertiesCard, mesh_info: MeshInfo) -> None:
        """分配属性卡中的外部量和派生量"""
        region_attributes = mesh_info.region_attributes
        zone_attributes = mesh_info.boundary_attributes
        for configs in (properties.external_property, properties.derived_property):
            for property_config in configs:
                for region in property_config.region:
                    self.properties_system.assign(region_attributes[region.name], f"{property_config.name}.{region.model}", region.parameters)
                for zone in property_config.zone:
                    self.properties_system.assign(zone_attributes[zone.name], f"{property_config.name}.{zone.model}", zone.parameters)
                if property_config.global_ is not None:
                    global_config = property_config.global_
                    self.properties_system.assign(0, f"{property_config.name}.{global_config.model}", global_config.parameters)

    def assign_material_card(self, material: MaterialCard, mesh_info: MeshInfo) -> None:
        """按区域分配材料卡中的全部物性"""
        materials = {item.name: item for item in material.mat}
        for assignment in material.assign.region:
            for property_config in materials[assignment.mat].properties:
                self.properties_system.assign(mesh_info.region_attributes[assignment.name], f"{property_config.property_name}.{property_config.model}", property_config.parameters)

    def assign_radioactivity_card(self, radioactivity: RadioactivityCard | None) -> None:
        """在分配模型前，把核素信息存为全局预计算属性和公共构造参数"""
        if radioactivity is not None:
            self.properties_system.set_precomputed("nuclide_name", radioactivity.nuclide.name)
            self.properties_system.set_precomputed("nuclide_symbol", radioactivity.nuclide.element_symbol)

    def update_external(self, time: float, time_step: float) -> None:
        """更新给定时刻的外部属性"""
        self.properties_system.update_external(time, time_step)

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
