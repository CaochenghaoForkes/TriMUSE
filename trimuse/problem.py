from __future__ import annotations
from typing import TYPE_CHECKING

from _core import ThermalProblem # type: ignore

if TYPE_CHECKING:
    from _core import FemSystem, PropertiesSystem, SolverSystem
    from .geometry.topology import MeshInfo
    from .input.thermal.config import ThermalCard
    from .input.xml_loader import InputCards

ThermalConfigEntry = tuple[int, str, dict[str, float | list[float]]]

class Thermal:
    """整理热问题配置, 创建并调用 C++ 热问题对象"""

    field_state = "temperature"

    def __init__(self, input_cards: InputCards, mesh_info: MeshInfo, fem: FemSystem, properties: PropertiesSystem, solver: SolverSystem) -> None:
        problem_type = next(field.model for field in input_cards.simulation.problem.field if field.name == "thermal")
        initial_configs = self.prepare_initial(input_cards.thermal.initial, mesh_info.region_attributes)
        source_configs = self.prepare_source(input_cards.thermal.source, mesh_info.region_attributes)
        boundary_configs = self.prepare_boundary(input_cards.thermal.boundary, mesh_info.boundary_attributes)
        self.thermal_problem = ThermalProblem(fem, properties, solver, problem_type, initial_configs, source_configs, boundary_configs)

    @staticmethod
    def prepare_initial(initial: ThermalCard.Initial, region_attributes: dict[str, int]) -> list[ThermalConfigEntry]:
        """将初温区域名称转换为网格区域编号"""
        return [(region_attributes[item.region], item.model, item.parameters) for item in initial.temperature]

    @staticmethod
    def prepare_source(source: ThermalCard.Source, region_attributes: dict[str, int]) -> list[ThermalConfigEntry]:
        """整理各区域的热源模型配置"""
        return [(region_attributes[item.name], item.model, item.parameters) for item in source.region]

    @staticmethod
    def prepare_boundary(boundaries: list[ThermalCard.Boundary], boundary_attributes: dict[str, int]) -> list[ThermalConfigEntry]:
        """将热边界名称转换为网格边界编号"""
        return [(boundary_attributes[item.name], item.model, item.parameters) for item in boundaries]

    def solve(self, time_step: float = 0.0, theta: float = 1.0) -> None:
        """调用统一求解入口, 问题类型由 C++ 对象保存"""
        self.thermal_problem.solve(time_step, theta)
