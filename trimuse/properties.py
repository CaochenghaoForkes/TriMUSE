from __future__ import annotations
from typing import TYPE_CHECKING

from _core import PropertiesSystem # type: ignore

if TYPE_CHECKING:
    from .fem import FemManager
    from .geometry.topology import MeshInfo
    from .input.xml_loader import InputCards
    from .input.material.config import MaterialCard
    from .input.properties.config import PropertiesCard

class PropertyModelExpander:
    """将上层属性模型展开为实际配置的属性模型组合"""

    @staticmethod
    def expand(name: str, model: str, parameters: dict[str, float | list[float]]) -> list[tuple[str, dict[str, float | list[float]]]]:
        if name == "assembly.cumulative_co_release":
            return PropertyModelExpander.expand_panama_co_release(model, parameters)
        if name == "assembly.cumulative_fission_gas_release":
            return PropertyModelExpander.expand_booth_fission_gas_release(model, parameters)
        return [(f"{name}.{model}", parameters)]

    @staticmethod
    def expand_booth_fission_gas_release(model: str, parameters: dict[str, float | list[float]]) -> list[tuple[str, dict[str, float | list[float]]]]:
        """Booth 参数交给状态模型，累计释放量模型只汇总状态"""
        states = {
            "common.from_local_booth_fission_gas_release": "assembly.local_booth_fission_gas_release_state",
            "common.from_homogenized_booth_fission_gas_release": "assembly.homogenized_booth_fission_gas_release_state",
        }
        return [(f"{states[model]}.common.booth", parameters), (f"assembly.cumulative_fission_gas_release.{model}", {})]

    @staticmethod
    def expand_panama_co_release(model: str, parameters: dict[str, float | list[float]]) -> list[tuple[str, dict[str, float | list[float]]]]:
        """停堆时刻交给 OPF 模型，累计 CO 量模型负责盘存换算"""
        states = {
            "common.from_local_panama_opf": "assembly.local_panama_opf",
            "common.from_homogenized_panama_opf": "assembly.homogenized_panama_opf",
        }
        return [(f"{states[model]}.common.panama", parameters), (f"assembly.cumulative_co_release.{model}", {})]

class PropertiesManager:
    """管理 C++ 属性系统的模型配置与更新"""

    def __init__(self, input_cards: InputCards, fem_manager: FemManager) -> None:
        """创建属性系统, 按材料区域或边界分区编号安排模型"""
        mesh_info = fem_manager.mesh_info
        field_names = sorted({field.name for field in input_cards.simulation.problem.field})
        precomputed_parameters = self.prepare_precomputed_parameters(input_cards)
        self.properties_system = PropertiesSystem(fem_manager.fem_system, field_names, precomputed_parameters)
        self.assign_properties_card(input_cards.properties, mesh_info)
        self.assign_material_card(input_cards.material, mesh_info)
        self.properties_system.prepare_dependencies() # 补齐固定关系属性并准备依赖

    @staticmethod
    def prepare_precomputed_parameters(input_cards: InputCards) -> dict[str, str | float | list[float]]:
        """汇总各物理问题卡的 parameters"""
        parameters = {}
        for card in (input_cards.thermal, input_cards.mechanics, input_cards.radioactivity):
            if card is not None:
                parameters.update(card.parameters)
        return parameters

    def assign_properties_card(self, properties: PropertiesCard, mesh_info: MeshInfo) -> None:
        """展开绑定模型并分配属性卡中的各类属性"""
        region_attributes = mesh_info.region_attributes
        zone_attributes = mesh_info.boundary_attributes
        for configs in (properties.external_property, properties.derived_property, properties.assembly_property):
            for property_config in configs:
                for region in property_config.region:
                    parameters = self.fill_region_parameters(property_config.name, region, region_attributes)
                    self.assign_property(region_attributes[region.name], property_config.name, region.model, parameters)
                for zone in property_config.zone:
                    self.assign_property(zone_attributes[zone.name], property_config.name, zone.model, zone.parameters)
                for interface in property_config.interface:
                    self.assign_property(zone_attributes[interface.name], property_config.name, interface.model, interface.parameters)
                if property_config.global_ is not None:
                    global_config = property_config.global_
                    self.assign_property(0, property_config.name, global_config.model, global_config.parameters)

    @staticmethod
    def fill_region_parameters(name: str, region: PropertiesCard.Region, region_attributes: dict[str, int]) -> dict[str, float | list[float]]:
        """统一处理区域属性模型需要的参数转换，保留原始输入"""
        parameters = dict(region.parameters)
        if name == "assembly.internal_gas_pressure" and region.model in {"common.ideal_gas_from_co_fission_gas_release", "common.rk_from_co_fission_gas_release"}:
            parameters["source_region"] = float(region_attributes[parameters["source_region"]])
        return parameters

    def assign_property(self, attribute: int, name: str, model: str, parameters: dict[str, float | list[float]]) -> None:
        """将上层模型及其绑定的下层模型分配给同一位置"""
        for model_name, model_parameters in PropertyModelExpander.expand(name, model, parameters):
            self.properties_system.assign(attribute, model_name, model_parameters)

    def assign_material_card(self, material: MaterialCard, mesh_info: MeshInfo) -> None:
        """按区域分配材料卡中的全部物性"""
        materials = {item.name: item for item in material.mat}
        for assignment in material.assign.region:
            for property_config in materials[assignment.mat].properties:
                self.assign_property(mesh_info.region_attributes[assignment.name], property_config.property_name, property_config.model, property_config.parameters)

    def update_field_independent_properties(self, time: float, time_step: float) -> None:
        """更新时间及不依赖场的属性，最后保存依赖快照"""
        self.properties_system.update_field_independent_properties(time, time_step)

    def update_field_dependent_properties(self, field_name: str) -> None:
        """采样正式场，再依次更新依赖该场的派生量、材料与组装属性"""
        self.properties_system.update_field_dependent_properties(field_name)

    def accept_current_state(self) -> None:
        """保存已接受时刻的完整属性快照"""
        self.properties_system.accept_current_state()
