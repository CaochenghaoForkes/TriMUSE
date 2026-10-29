from __future__ import annotations
from typing import TYPE_CHECKING

from _core import ThermalProblem, RadioactivityProblem # type: ignore

if TYPE_CHECKING:
    from _core import FemSystem, PropertiesSystem, SolverSystem, OutputManager
    from .geometry.topology import MeshInfo
    from .input.thermal.config import ThermalCard
    from .input.radioactivity.config import RadioactivityCard
    from .input.xml_loader import InputCards

ProblemConfigEntry = tuple[int, str, dict[str, float | list[float]]]

class Thermal:
    """整理热问题配置, 创建并调用 C++ 热问题对象"""

    field_state = "temperature"

    def __init__(self, input_cards: InputCards, mesh_info: MeshInfo, fem: FemSystem, properties: PropertiesSystem, solver: SolverSystem, output: OutputManager) -> None:
        problem_type = next(field.model for field in input_cards.simulation.problem.field if field.name == self.field_state)
        initial_configs = self.prepare_initial(input_cards.thermal.initial, mesh_info.region_attributes)
        source_configs = self.prepare_source(input_cards.thermal.source, mesh_info.region_attributes)
        boundary_configs = self.prepare_boundary(input_cards.thermal.boundary, mesh_info.boundary_attributes)
        geometry = input_cards.geometry_mesh.geometry
        center = getattr(geometry, geometry.source).center
        interface_configs = []
        for item in input_cards.thermal.interfaces.interface:
            parameters = item.parameters
            if item.model == "local_from_constant_resistance":
                # 局部分区沿用几何中心，内部坐标使用 m；不修改用户输入参数。
                parameters = {**parameters, "center": [center.x, center.y, center.z]}
            interface_configs.append((mesh_info.boundary_attributes[item.name], item.model, parameters))
        self.thermal_problem = ThermalProblem(fem, properties, solver, output, problem_type,
                                             initial_configs, source_configs, boundary_configs, interface_configs)

    @staticmethod
    def prepare_initial(initial: ThermalCard.Initial, region_attributes: dict[str, int]) -> list[ProblemConfigEntry]:
        """将初温区域名称转换为网格区域编号"""
        return [(region_attributes[item.region], item.model, item.parameters) for item in initial.temperature]

    @staticmethod
    def prepare_source(source: ThermalCard.Source, region_attributes: dict[str, int]) -> list[ProblemConfigEntry]:
        """整理各区域的热源模型配置"""
        return [(region_attributes[item.name], item.model, item.parameters) for item in source.region]

    @staticmethod
    def prepare_boundary(boundaries: list[ThermalCard.Boundary], boundary_attributes: dict[str, int]) -> list[ProblemConfigEntry]:
        """将热边界名称转换为网格边界编号"""
        return [(boundary_attributes[item.name], item.model, item.parameters) for item in boundaries]

    def solve(self, time_step: float = 0.0, theta: float = 1.0) -> None:
        """调用统一求解入口, 问题类型由 C++ 对象保存"""
        self.thermal_problem.solve(time_step, theta)

class Radioactivity:
    """整理单核素配置, 创建并调用 C++ 扩散与衰变问题对象"""

    field_state = "concentration"

    def __init__(self, input_cards: InputCards, mesh_info: MeshInfo, fem: FemSystem, properties: PropertiesSystem, solver: SolverSystem, output: OutputManager) -> None:
        problem_type = next(field.model for field in input_cards.simulation.problem.field if field.name == self.field_state)
        card = input_cards.radioactivity
        initial_configs = self.prepare_initial(card.initial, mesh_info.region_attributes)
        source_configs = self.prepare_source(card.source, mesh_info.region_attributes)
        boundary_configs = self.prepare_boundary(card.boundary, mesh_info.boundary_attributes)
        interface_configs = [(mesh_info.boundary_attributes[item.name], item.model, item.parameters)
                             for item in card.interfaces.interface]
        self.radioactivity_problem = RadioactivityProblem(
            fem, properties, solver, output, problem_type, card.nuclide.decay_constant,
            initial_configs, source_configs, boundary_configs, interface_configs
        )

    @staticmethod
    def prepare_initial(initial: RadioactivityCard.Initial, region_attributes: dict[str, int]) -> list[ProblemConfigEntry]:
        """区域总盘存已由输入层换算为 atom, 除以体积由 C++ 初始化负责"""
        return [(region_attributes[item.region], item.model, item.parameters) for item in initial.concentration]

    @staticmethod
    def prepare_source(source: RadioactivityCard.Source, region_attributes: dict[str, int]) -> list[ProblemConfigEntry]:
        return [(region_attributes[item.name], item.model, item.parameters) for item in source.region]

    @staticmethod
    def prepare_boundary(boundaries: list[RadioactivityCard.Boundary], boundary_attributes: dict[str, int]) -> list[ProblemConfigEntry]:
        return [(boundary_attributes[item.name], item.model, item.parameters) for item in boundaries]

    def solve(self, time_step: float = 0.0, theta: float = 1.0) -> None:
        self.radioactivity_problem.solve(time_step, theta)
