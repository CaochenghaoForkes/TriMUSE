import math

from .config import SimulationCard

class SimulationCardExamine:
    """检查阶段列表和调度引用, 不预设后续场间交换的执行算法"""

    @classmethod
    def examine(cls, card: SimulationCard) -> None:
        """检查时间配置, 求解选项及场引用"""
        problem = card.problem
        stage_count = len(problem.time_span)
        for name, values in (("time_span", problem.time_span), ("min_time_step", problem.min_time_step)):
            if not values or len(values) != stage_count or not all(math.isfinite(value) and value > 0 for value in values):
                raise ValueError(f"{name} 必须包含各阶段的有限正数")
        intervals = card.output.write_interval
        if len(intervals) != stage_count or any(not math.isfinite(value) or value == 0 or value < 0 and value % 1 != 0 for value in intervals):
            raise ValueError("output.write_interval 必须按阶段填写: 正数为时间间隔, 负整数为步数间隔, 不能为零")
        cls.check_intervals(card.coupling.exchange_interval, stage_count, "exchange_interval")
        field_names = [item.name for item in problem.field]
        solver_names = [item.name for item in card.solver.field]
        if len(set(field_names)) != len(field_names) or len(set(solver_names)) != len(solver_names):
            raise ValueError("problem 或 solver 中的场名称不能重复")
        if set(field_names) != set(solver_names):
            raise ValueError("problem 与 solver 的场名称必须对应")
        for item in problem.field:
            if item.model not in ("steady", "transient"):
                raise ValueError(f"{item.name}: model 必须为 [steady,transient]")
            if item.model == "steady" and item.theta is not None:
                raise ValueError(f"{item.name}: steady 场不使用 theta, 请删除该属性")
            if item.theta is not None and item.theta not in (0.0, 0.5, 1.0):
                raise ValueError(f"{item.name}: theta 必须为 0、0.5 或 1")
        if card.solver.quadrature.domain_order < 1 or card.solver.quadrature.boundary_order < 1:
            raise ValueError("solver.quadrature: domain_order 和 boundary_order 必须为正整数")
        for item in card.solver.field:
            if item.linear_solver.method != "cg":
                raise ValueError(f"{item.name}: 当前线性求解使用 cg")
            if item.linear_solver.preconditioner not in ("none", "jacobi", "symmetric_gauss_seidel"):
                raise ValueError(f"{item.name}: preconditioner 必须为 [none,jacobi,symmetric_gauss_seidel]")
            if item.fem_space.family != "H1" or item.fem_space.order < 1:
                raise ValueError(f"{item.name}: 初版使用 H1 空间, order 至少为 1")
            if item.linear_solver.max_iterations < 1:
                raise ValueError(f"{item.name}: max_iterations 必须为正整数")
            cls.check_tolerance(item.linear_solver.relative_tolerance, item.name)
            if item.nonlinear_solver.method != "picard":
                raise ValueError(f"{item.name}: 当前非线性求解使用 picard")
            if item.nonlinear_solver.max_iterations < 1:
                raise ValueError(f"{item.name}: 非线性 max_iterations 必须为正整数")
            cls.check_tolerance(item.nonlinear_solver.relative_tolerance, item.name)
            if not math.isfinite(item.nonlinear_solver.absolute_tolerance) or item.nonlinear_solver.absolute_tolerance < 0:
                raise ValueError(f"{item.name}: 非线性 absolute_tolerance 必须为有限非负温度增量")
        for step in card.coupling.step:
            if step.method not in ("single", "staggered"):
                raise ValueError("耦合 method 必须为 [single,staggered]")
            if len(set(step.fields)) != len(step.fields):
                raise ValueError("同一个耦合 step 中的场不能重复")
            unknown = set(step.fields) - set(field_names)
            if unknown:
                raise ValueError(f"耦合步骤引用未定义的场: {', '.join(sorted(unknown))}")
            if step.method == "staggered":
                if step.max_iterations < 1:
                    raise ValueError("耦合 max_iterations 必须为正整数")
                cls.check_tolerance(step.relative_tolerance, "coupling")

    @staticmethod
    def check_intervals(values: list[int], stage_count: int, name: str) -> None:
        """每个阶段填写一个正整数倍数"""
        if len(values) != stage_count or any(value < 1 for value in values):
            raise ValueError(f"{name} 必须包含各阶段的正整数")

    @staticmethod
    def check_tolerance(value: float, name: str) -> None:
        """相对容差必须在零与一之间"""
        if not 0 < value < 1:
            raise ValueError(f"{name}: relative_tolerance 必须在 (0,1) 内")
