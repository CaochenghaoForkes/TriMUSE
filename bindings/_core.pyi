"""TriMUSE C++ 扩展的类型声明, 与 module.cpp 中的绑定同步维护"""

ThermalConfigEntry = tuple[int, str, dict[str, float | list[float]]]
LinearSolverEntry = tuple[str, str, float, int]
NonlinearSolverEntry = tuple[str, float, float, int]
FieldSolverEntry = tuple[str, LinearSolverEntry, NonlinearSolverEntry]

class FemSystem:
    """公共网格 求积规则与各场有限元对象"""

    def __init__(self, mesh_path: str, domain_quadrature_order: int, boundary_quadrature_order: int) -> None: ...

    def add_field(self, name: str, order: int, vector_dimension: int = 1) -> None: ...

class OutputManager:
    """按场名管理独立的 ParaView 时间序列"""

    def __init__(self, fem_system: FemSystem, output_directory: str) -> None: ...

    def add_field(self, field_name: str, directory_name: str) -> None: ...

    def save(self, field_name: str, cycle: int, time: float, time_step: float) -> None: ...

class PropertiesSystem:
    """外部 派生与材料属性的统一配置及更新接口"""

    def __init__(self, fem_system: FemSystem) -> None: ...

    def assign(self, attribute: int, model_name: str, parameters: dict[str, float | list[float]]) -> None: ...

    def prepare_dependencies(self) -> None: ...

    def update_external(self, time: float) -> None: ...

    def update_derived(self) -> None: ...

    def update_material(self) -> None: ...

    def update_field_dependent_materials(self, field_name: str) -> None: ...

    def accept_current_state(self) -> None: ...

class SolverSystem:
    """各场线性与非线性求解配置"""

    def __init__(self, configs: list[FieldSolverEntry]) -> None: ...

class ThermalProblem:
    """初始化温度场并统一调度热问题求解"""

    def __init__(self, fem_system: FemSystem, properties_system: PropertiesSystem,
                 solver_system: SolverSystem, problem_type: str,
                 initial_configs: list[ThermalConfigEntry], source_configs: list[ThermalConfigEntry],
                 boundary_configs: list[ThermalConfigEntry]) -> None: ...

    def solve(self, time_step: float = 0.0, theta: float = 1.0) -> None: ...
