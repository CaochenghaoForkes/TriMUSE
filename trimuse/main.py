from pathlib import Path
import sys
import numpy as np

project_directory = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(project_directory))
sys.path.insert(0, str(project_directory / "build" / "bindings"))
from _core import OutputManager
from trimuse.input import load_input_cards
from trimuse.geometry.builder import GeometryMeshBuilder
from trimuse.fem import FemManager
from trimuse.properties import PropertiesManager
from trimuse.solver import SolverManager
from trimuse.problem import Thermal
from trimuse.coupling import CouplingManager

if __name__ == "__main__":

    # 读取输入
    chief_path = project_directory / "cases/flux/case.xml"
    input_cards = load_input_cards(chief_path)

    # 创建随机数生成器
    rng = np.random.default_rng(input_cards.chief.random_seed)

    # 生成网格
    mesh_info = GeometryMeshBuilder.build(input_cards.geometry_mesh, input_cards.chief.output.directory, rng)

    # 创建公共网格 求积规则与各场有限元对象
    fem_manager = FemManager(mesh_info, input_cards.simulation.solver)

    # 安排外部属性 派生属性 材料属性的模型
    properties_manager = PropertiesManager(input_cards, mesh_info, fem_manager.fem_system)

    # 按场安排线性与非线性求解配置
    solver_manager = SolverManager(input_cards.simulation.solver)

    # 创建热问题对象, 同时初始化温度场
    thermal_problem = Thermal(input_cards, mesh_info, fem_manager.fem_system, properties_manager.properties_system, solver_manager.solver_system)

    # 保存 thermal/temperature 的 ParaView 时间序列
    output_manager = OutputManager(fem_manager.fem_system, input_cards.chief.output.directory)

    # 按阶段推进时间 更新属性并求解热问题
    coupling_manager = CouplingManager(input_cards.simulation, properties_manager, {"thermal": thermal_problem}, output_manager)
    coupling_manager.run()
