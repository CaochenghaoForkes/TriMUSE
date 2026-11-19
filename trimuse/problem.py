from __future__ import annotations
from typing import TYPE_CHECKING

from _core import ThermalProblem, RadioactivityProblem, MechanicsProblem # type: ignore

if TYPE_CHECKING:
    from .fem import FemManager
    from .output import OutputManager
    from .properties import PropertiesManager
    from .solver import SolverManager
    from .input.properties.config import PropertiesCard
    from .input.xml_loader import InputCards
    from .geometry.topology import MeshInfo

ProblemConfigEntry = tuple[int, str, dict[str, float | list[float]]]

def expand_locations(entries, names) -> dict: # 将输入中的名称列表和 residual，展开成“单个位置名称 → 配置对象”的字典
    """先展开全部显式位置，再将 residual 分配给其余位置"""
    expanded, residual = {}, None # expanded 保存展开结果；residual 暂存剩余位置使用的配置，没有指定时为 None
    for location, entry in entries:
        if location == "residual":
            residual = entry
        else:
            for name in location.split():
                expanded[name] = entry # "kernel buffer" -> expanded["kernel"] = 配置A expanded["buffer"] = 配置A
    if residual is not None:
        for name in names:
            expanded.setdefault(name, residual) # setdefault: 如果已经有 name 就什么也不做 如果没有就填入 default 的配置
    return expanded

def prepare_configs(card, input_cards: InputCards, mesh_info: MeshInfo, terms: tuple) -> tuple[dict, dict, dict]: # 将一张问题卡中的区域、外边界、内部界面配置，一次整理成 C++ 接收的三个字典
    """一次整理域、外边界和内部界面的组装配置，未启用项保留空列表"""
    geometry = input_cards.geometry_mesh.geometry
    layers = getattr(geometry, geometry.source).regions.region
    region_names = list(mesh_info.region_attributes) # 从名称—编号字典中取得全部区域名称 如 ["kernel", "buffer", "ipyc", "sic", "opyc"]
    zone_names = [zone.name for zone in geometry.zones.zone] + [geometry.zones.residual_zone] # 取得显式划分的外表面分区名称，再加上几何中剩余外表面的名称
    interface_names = [layer.outer_boundary.name for layer in layers[:-1]] # 取得内部材料界面的名称 layers[:-1] 排除最外层
    groups = (
        (card.regions.region, region_names, mesh_info.region_attributes),
        (card.boundaries.boundary, zone_names, mesh_info.boundary_attributes),
        (card.interfaces.interface, interface_names, mesh_info.boundary_attributes),
    ) # 每组包含 (输入配置列表, 全部位置名称, 名称到网格编号的字典)
    configs = []
    for (entries, names, attributes), group_terms in zip(groups, terms): # 逐组转化
        group = {term: [] for term in group_terms} # 为当前类别的每种组装项建立空列表 group = {"conduction": [],"capacity": [],"source": []}
        for name, entry in expand_locations(((item.name, item) for item in entries), names).items(): # ((item.name, item) for item in entries) 得到位置字符串、配置对象 entries = [("residual", 配置B),("kernel buffer", 配置A)] -> {"kernel": 配置A,"buffer": 配置A,"ipyc": 配置B,"sic": 配置B,"opyc": 配置B}
            for model in entry.model:
                group[model.name].append((attributes[name], model.model_name, {})) # group["conduction"].append((1, "fourier", {}))
        configs.append(group)
    return tuple(configs) # (regions字典, boundaries字典, interfaces字典)

def prepare_initial(entries, region_attributes: dict[str, int]) -> list[ProblemConfigEntry]:
    """展开初值区域并转换编号，初值参数直接传给 C++"""
    configs = []
    for name, item in expand_locations(((item.region, item) for item in entries), region_attributes).items():
        configs.append((region_attributes[name], item.model, item.parameters))
    return configs

class Thermal:
    """整理热问题配置, 创建并调用 C++ 热问题对象"""

    field_state = "temperature"

    def __init__(self, input_cards: InputCards, fem_manager: FemManager, properties_manager: PropertiesManager, solver_manager: SolverManager, output_manager: OutputManager) -> None:
        problem_type = next(field.model for field in input_cards.simulation.problem.field if field.name == self.field_state)
        card = input_cards.thermal
        mesh_info = fem_manager.mesh_info
        initial_configs = prepare_initial(card.initial.temperature, mesh_info.region_attributes)
        regions, boundaries, interfaces = prepare_configs(card, input_cards, mesh_info, (('conduction', 'capacity', 'source'), ('heat_boundary',), ('interface_heat_transfer',)))
        self.fill_interface_heat_transfer_parameters(interfaces["interface_heat_transfer"], mesh_info.boundary_attributes, input_cards.properties)
        self.thermal_problem = ThermalProblem(fem_manager.fem_system, properties_manager.properties_system, solver_manager.solver_system, output_manager.field_output, problem_type, initial_configs, regions, boundaries, interfaces)

    @staticmethod
    def fill_interface_heat_transfer_parameters(configs: list[ProblemConfigEntry], boundary_attributes: dict[str, int], properties: PropertiesCard) -> None:
        """从属性卡补充热阻角域参数，供 C++ 初始化完美接触约束"""
        angle_names = {"theta_mins", "theta_maxs", "thetas", "phis", "angular_distance_mins", "angular_distance_maxs"}
        angles_by_interface = {}
        for property_config in properties.assembly_property:
            if property_config.name != "assembly.interface_thermal_resistance":
                continue
            for interface in property_config.interface:
                angles = {}
                for name, value in interface.parameters.items():
                    if name in angle_names:
                        angles[name] = value
                angles_by_interface[boundary_attributes[interface.name]] = angles
        for index, (attribute, model, parameters) in enumerate(configs):
            if model == "interface_thermal_resistance":
                configs[index] = (attribute, model, angles_by_interface[attribute])

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
        initial_configs = prepare_initial(card.initial.concentration, mesh_info.region_attributes)
        regions, boundaries, interfaces = prepare_configs(card, input_cards, mesh_info, (('diffusion', 'capacity', 'source'), ('nuclide_boundary',), ('interface_nuclide_transfer',)))
        self.radioactivity_problem = RadioactivityProblem(fem_manager.fem_system, properties_manager.properties_system, solver_manager.solver_system, output_manager.field_output, problem_type, initial_configs, regions, boundaries, interfaces)

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
        regions, boundaries, interfaces = prepare_configs(card, input_cards, mesh_info, (('stiffness', 'thermal_expansion', 'source'), ('mechanics_boundary',), ('interface_contact',)))
        self.mechanics_problem = MechanicsProblem(fem_manager.fem_system, properties_manager.properties_system, solver_manager.solver_system, output_manager.field_output, regions, boundaries, interfaces)

    def solve(self, time_step: float = 0.0, theta: float = 1.0) -> None:
        """保持统一调度接口，准静态方程本身不使用时间离散参数"""
        self.mechanics_problem.solve()

    def save_output(self, cycle: int, time: float, time_step: float) -> None:
        self.mechanics_problem.save_output(cycle, time, time_step)
