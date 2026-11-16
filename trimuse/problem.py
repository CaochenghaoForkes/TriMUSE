from __future__ import annotations
from typing import TYPE_CHECKING

from _core import ThermalProblem, RadioactivityProblem, MechanicsProblem # type: ignore

if TYPE_CHECKING:
    from .fem import FemManager
    from .output import OutputManager
    from .properties import PropertiesManager
    from .solver import SolverManager
    from .input.mechanics.config import MechanicsCard
    from .input.thermal.config import ThermalCard
    from .input.radioactivity.config import RadioactivityCard
    from .input.xml_loader import InputCards

ProblemConfigEntry = tuple[int, str, dict[str, float | list[float]]]

class Thermal:
    """整理热问题配置, 创建并调用 C++ 热问题对象"""

    field_state = "temperature"

    def __init__(self, input_cards: InputCards, fem_manager: FemManager, properties_manager: PropertiesManager, solver_manager: SolverManager, output_manager: OutputManager) -> None:
        problem_type = next(field.model for field in input_cards.simulation.problem.field if field.name == self.field_state)
        card = input_cards.thermal
        mesh_info = fem_manager.mesh_info
        initial_configs = self.prepare_initial(card.initial, mesh_info.region_attributes)
        source_configs = self.prepare_source(card.source, mesh_info.region_attributes)
        boundary_configs = self.prepare_boundary(card.boundary, mesh_info.boundary_attributes)
        interface_configs = self.prepare_interfaces(card.interfaces, mesh_info.boundary_attributes)
        self.thermal_problem = ThermalProblem(
            fem_manager.fem_system, properties_manager.properties_system,
            solver_manager.solver_system, output_manager.field_output, problem_type,
            initial_configs, source_configs, boundary_configs, interface_configs,
        )

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

    @staticmethod
    def prepare_interfaces(interfaces: ThermalCard.Interfaces, boundary_attributes: dict[str, int]) -> list[ProblemConfigEntry]:
        """将热界面名称转换为网格物理组编号"""
        return [(boundary_attributes[item.name], item.model, item.parameters) for item in interfaces.interface]

    def solve(self, time_step: float = 0.0, theta: float = 1.0) -> None:
        """调用统一求解入口, 问题类型由 C++ 对象保存"""
        self.thermal_problem.solve(time_step, theta)

    def save_output(self, cycle: int, time: float, time_step: float) -> None:
        self.thermal_problem.save_output(cycle, time, time_step)

class Radioactivity:
    """整理单核素配置, 创建并调用 C++ 扩散与衰变问题对象"""

    field_state = "concentration"

    def __init__(self, input_cards: InputCards, fem_manager: FemManager, properties_manager: PropertiesManager, solver_manager: SolverManager, output_manager: OutputManager) -> None:
        problem_type = next(field.model for field in input_cards.simulation.problem.field if field.name == self.field_state)
        card = input_cards.radioactivity
        mesh_info = fem_manager.mesh_info
        initial_configs = self.prepare_initial(card.initial, mesh_info.region_attributes)
        source_configs = self.prepare_source(card.source, mesh_info.region_attributes)
        boundary_configs = self.prepare_boundary(card.boundary, mesh_info.boundary_attributes)
        interface_configs = self.prepare_interfaces(card.interfaces, mesh_info.boundary_attributes)
        self.radioactivity_problem = RadioactivityProblem(
            fem_manager.fem_system, properties_manager.properties_system,
            solver_manager.solver_system, output_manager.field_output, problem_type,
            card.nuclide.decay_constant, initial_configs, source_configs, boundary_configs, interface_configs,
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

    @staticmethod
    def prepare_interfaces(interfaces: RadioactivityCard.Interfaces, boundary_attributes: dict[str, int]) -> list[ProblemConfigEntry]:
        """将放射性输运界面名称转换为网格物理组编号"""
        return [(boundary_attributes[item.name], item.model, item.parameters) for item in interfaces.interface]

    def solve(self, time_step: float = 0.0, theta: float = 1.0) -> None:
        self.radioactivity_problem.solve(time_step, theta)

    def save_output(self, cycle: int, time: float, time_step: float) -> None:
        self.radioactivity_problem.save_output(cycle, time, time_step)

class Mechanics:
    """整理力学配置，每个调度时刻求解一次准静态平衡"""

    field_state = "displacement"

    def __init__(self, input_cards: InputCards, fem_manager: FemManager, properties_manager: PropertiesManager, solver_manager: SolverManager, output_manager: OutputManager) -> None:
        card = input_cards.mechanics
        mesh_info = fem_manager.mesh_info
        source_configs = self.prepare_source(card.source, mesh_info.region_attributes)
        boundary_configs = self.prepare_boundary(card.boundary, mesh_info.boundary_attributes)
        interface_configs = self.prepare_interfaces(card.interfaces, mesh_info.boundary_attributes)
        self.mechanics_problem = MechanicsProblem(
            fem_manager.fem_system, properties_manager.properties_system,
            solver_manager.solver_system, output_manager.field_output,
            source_configs, boundary_configs, interface_configs,
        )

    @staticmethod
    def prepare_source(source: MechanicsCard.Source, region_attributes: dict[str, int]) -> list[ProblemConfigEntry]:
        """整理各区域的体积力源项配置"""
        return [(region_attributes[item.name], item.model, item.parameters) for item in source.region]

    @staticmethod
    def prepare_boundary(boundaries: list[MechanicsCard.Boundary], boundary_attributes: dict[str, int]) -> list[ProblemConfigEntry]:
        """将力学外边界名称转换为网格边界编号"""
        return [(boundary_attributes[item.name], item.model, item.parameters) for item in boundaries]

    @staticmethod
    def prepare_interfaces(interfaces: MechanicsCard.Interfaces, boundary_attributes: dict[str, int]) -> list[ProblemConfigEntry]:
        """将力学界面名称转换为网格物理组编号"""
        return [(boundary_attributes[item.name], item.model, item.parameters) for item in interfaces.interface]

    def solve(self, time_step: float = 0.0, theta: float = 1.0) -> None:
        """保持统一调度接口，准静态方程本身不使用时间离散参数"""
        self.mechanics_problem.solve()

    def save_output(self, cycle: int, time: float, time_step: float) -> None:
        self.mechanics_problem.save_output(cycle, time, time_step)
