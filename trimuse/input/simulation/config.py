from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from xml.etree import ElementTree as ET
import sys

from ..xml_utils import XmlUtils
from .template import SimulationCardTemplate

@dataclass
class SimulationCard:
    """分阶段问题定义, 求解器与耦合调度配置, 时间统一为 s"""

    @dataclass
    class Problem:
        """time_span 为各阶段持续时间, 默认从零开始累计"""

        @dataclass
        class Field:
            """steady 为调度时刻重新求解的准稳态, transient 为瞬态"""

            name: str
            model: str  # [steady,transient]
            theta: float | None = None  # 仅瞬态使用; 省略时按 1 处理

        time_span: list[float]
        min_time_step: list[float]
        field: list[Field] = field(default_factory=list)

    @dataclass
    class Solver:
        """公共求积规则与各物理场的数值配置"""

        @dataclass
        class FemSpace:
            """有限元空间, 不改变几何网格阶次"""

            family: str = "H1"
            order: int = 1

        @dataclass
        class Quadrature:
            """域与边界的求积阶次"""

            domain_order: int
            boundary_order: int

        @dataclass
        class LinearSolver:
            """初版线性求解配置"""

            method: str = "cg"
            preconditioner: str = "none"
            relative_tolerance: float = 1e-8
            max_iterations: int = 1000

        @dataclass
        class NonlinearSolver:
            """Picard 温度增量收敛配置"""

            method: str = "picard"
            relative_tolerance: float = 1e-8
            absolute_tolerance: float = 1e-8
            max_iterations: int = 50

        @dataclass
        class Field:
            """以场名称关联问题定义"""

            name: str
            fem_space: SimulationCard.Solver.FemSpace = field(default_factory=lambda: SimulationCard.Solver.FemSpace())
            linear_solver: SimulationCard.Solver.LinearSolver = field(default_factory=lambda: SimulationCard.Solver.LinearSolver())
            nonlinear_solver: SimulationCard.Solver.NonlinearSolver = field(default_factory=lambda: SimulationCard.Solver.NonlinearSolver())

        quadrature: Quadrature
        field: list[Field] = field(default_factory=list)

    @dataclass
    class Coupling:
        """交换间隔为基础时间步的倍数, step 按书写顺序执行"""

        @dataclass
        class Step:
            """fields 的书写顺序为组内求解顺序, 具体执行规则后续实现"""

            fields: list[str]
            method: str = "single"  # [single,staggered]
            max_iterations: int = 20
            relative_tolerance: float = 1e-6

        exchange_interval: list[int]
        step: list[Step] = field(default_factory=list)

    @dataclass
    class Output:
        """各阶段按基础时间步计数的保存间隔"""

        write_interval: list[int]

    problem: Problem
    solver: Solver
    coupling: Coupling
    output: Output

class SimulationCardLoader:
    """读取调度配置, 不在输入阶段执行时间推进或耦合计算"""

    @classmethod
    def load(cls, path: str | Path | None = None) -> SimulationCard:
        """空路径生成模板并退出, 否则读取配置"""
        if path is None or isinstance(path, str) and not path.strip():
            template_path = SimulationCardTemplate.write()
            print(f"仿真模板输入卡已生成: {template_path}")
            sys.exit()
        root = XmlUtils.get_root(path, "simulation")
        problem = cls.read_problem(XmlUtils.read_child(root, "problem"))
        solver = cls.read_solver(XmlUtils.read_child(root, "solver"))
        coupling = cls.read_coupling(XmlUtils.read_child(root, "coupling"))
        output = cls.read_output(XmlUtils.read_child(root, "output"))
        simulation_card = SimulationCard(problem=problem, solver=solver, coupling=coupling, output=output)
        return simulation_card

    @classmethod
    def read_problem(cls, element: ET.Element) -> SimulationCard.Problem:
        """读取阶段持续时间、公共基础时间步和各场问题类型"""
        time_span_element = XmlUtils.read_child(element, "time_span")
        time_span = XmlUtils.read_attribute(time_span_element, "value", list[float], required=True, quantity="time")
        min_time_step_element = XmlUtils.read_child(element, "min_time_step")
        min_time_step = XmlUtils.read_attribute(min_time_step_element, "value", list[float], required=True, quantity="time")
        fields = [cls.read_problem_field(child) for child in XmlUtils.read_children(element, "field", required=True)]
        problem = SimulationCard.Problem(time_span=time_span, min_time_step=min_time_step, field=fields)
        return problem

    @staticmethod
    def read_output(element: ET.Element) -> SimulationCard.Output:
        """读取各阶段的结果保存间隔"""
        interval_element = XmlUtils.read_child(element, "write_interval")
        write_interval = XmlUtils.read_attribute(interval_element, "value", list[int], required=True)
        output = SimulationCard.Output(write_interval=write_interval)
        return output

    @staticmethod
    def read_problem_field(element: ET.Element) -> SimulationCard.Problem.Field:
        """读取场的问题类型和瞬态时间格式"""
        XmlUtils.check_attributes(element, ("name", "model", "theta"))
        name = XmlUtils.read_attribute(element, "name", required=True)
        model = XmlUtils.read_attribute(element, "model", required=True)
        theta = XmlUtils.read_attribute(element, "theta", float)
        problem_field = SimulationCard.Problem.Field(name=name, model=model, theta=theta)
        return problem_field

    @classmethod
    def read_solver(cls, element: ET.Element) -> SimulationCard.Solver:
        """读取全场公共求积规则与各场求解配置"""
        quadrature = cls.read_quadrature(XmlUtils.read_child(element, "quadrature"))
        fields = [cls.read_solver_field(child) for child in XmlUtils.read_children(element, "field", required=True)]
        solver = SimulationCard.Solver(quadrature=quadrature, field=fields)
        return solver

    @classmethod
    def read_solver_field(cls, element: ET.Element) -> SimulationCard.Solver.Field:
        """未填写求解选项时使用默认配置"""
        name = XmlUtils.read_attribute(element, "name", required=True)
        fem_space = cls.read_fem_space(XmlUtils.read_child(element, "fem_space", required=False))
        linear_solver = cls.read_linear_solver(XmlUtils.read_child(element, "linear_solver", required=False))
        nonlinear_solver = cls.read_nonlinear_solver(XmlUtils.read_child(element, "nonlinear_solver", required=False))
        solver_field = SimulationCard.Solver.Field(name=name, fem_space=fem_space, linear_solver=linear_solver, nonlinear_solver=nonlinear_solver)
        return solver_field

    @staticmethod
    def read_fem_space(element: ET.Element | None) -> SimulationCard.Solver.FemSpace:
        """初版默认使用一阶 H1 空间"""
        family = XmlUtils.read_attribute(element, "family", default="H1")
        order = XmlUtils.read_attribute(element, "order", int, default=1)
        fem_space = SimulationCard.Solver.FemSpace(family=family, order=order)
        return fem_space

    @staticmethod
    def read_quadrature(element: ET.Element) -> SimulationCard.Solver.Quadrature:
        """读取必填的域与边界求积阶次"""
        domain_order = XmlUtils.read_attribute(element, "domain_order", int, required=True)
        boundary_order = XmlUtils.read_attribute(element, "boundary_order", int, required=True)
        return SimulationCard.Solver.Quadrature(domain_order=domain_order, boundary_order=boundary_order)

    @staticmethod
    def read_linear_solver(element: ET.Element | None) -> SimulationCard.Solver.LinearSolver:
        """读取线性求解方法和相对容差"""
        method = XmlUtils.read_attribute(element, "method", default="cg")
        preconditioner = XmlUtils.read_attribute(element, "preconditioner", default="none")
        relative_tolerance = XmlUtils.read_attribute(element, "relative_tolerance", float, default=1e-8)
        max_iterations = XmlUtils.read_attribute(element, "max_iterations", int, default=1000)
        linear_solver = SimulationCard.Solver.LinearSolver(method=method, preconditioner=preconditioner, relative_tolerance=relative_tolerance, max_iterations=max_iterations)
        return linear_solver

    @staticmethod
    def read_nonlinear_solver(element: ET.Element | None) -> SimulationCard.Solver.NonlinearSolver:
        """读取 Picard 最大轮数及相邻两轮的温度增量容差"""
        method = XmlUtils.read_attribute(element, "method", default="picard")
        relative_tolerance = XmlUtils.read_attribute(element, "relative_tolerance", float, default=1e-8)
        max_iterations = XmlUtils.read_attribute(element, "max_iterations", int, default=50)
        absolute_element = XmlUtils.read_child(element, "absolute_tolerance", required=False) if element is not None else None
        absolute_tolerance = XmlUtils.read_attribute(absolute_element, "value", float, quantity="temperature_difference", default=1e-8)
        return SimulationCard.Solver.NonlinearSolver(method=method, relative_tolerance=relative_tolerance, absolute_tolerance=absolute_tolerance, max_iterations=max_iterations)

    @classmethod
    def read_coupling(cls, element: ET.Element) -> SimulationCard.Coupling:
        """读取阶段交换间隔和有序步骤"""
        exchange_interval = XmlUtils.read_attribute(element, "exchange_interval", list[int], required=True)
        steps = [cls.read_coupling_step(child) for child in XmlUtils.read_children(element, "step", required=True)]
        coupling = SimulationCard.Coupling(exchange_interval=exchange_interval, step=steps)
        return coupling

    @staticmethod
    def read_coupling_step(element: ET.Element) -> SimulationCard.Coupling.Step:
        """保留场列表及迭代设置, single 时不使用迭代参数"""
        fields = XmlUtils.read_attribute(element, "fields", list[str], required=True)
        method = XmlUtils.read_attribute(element, "method", default="single")
        max_iterations = XmlUtils.read_attribute(element, "max_iterations", int, default=20)
        relative_tolerance = XmlUtils.read_attribute(element, "relative_tolerance", float, default=1e-6)
        step = SimulationCard.Coupling.Step(fields=fields, method=method, max_iterations=max_iterations, relative_tolerance=relative_tolerance)
        return step
