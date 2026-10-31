from __future__ import annotations
from pathlib import Path
import sys
import numpy as np

project_directory = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(project_directory))
sys.path.insert(0, str(project_directory / "build" / "bindings"))

from trimuse.input import load_input_cards
from trimuse.output import OutputManager
from trimuse.geometry.builder import GeometryMeshBuilder
from trimuse.fem import FemManager
from trimuse.properties import PropertiesManager
from trimuse.solver import SolverManager
from trimuse.problem import Thermal, Radioactivity
from trimuse.coupling import CouplingManager
from trimuse.postprocess.postprocess import PostprocessManager

def main(chief_path: str | Path) -> None:
    """读取总调度卡，依次执行网格生成、模拟与后处理"""
    # 读取输入
    chief_path = Path(chief_path).expanduser().resolve()
    input_cards = load_input_cards(chief_path)

    # 创建输出对象
    output_manager = OutputManager(input_cards)

    # 创建随机数生成器
    rng = np.random.default_rng(input_cards.chief.random_seed)

    # 保存可重新读取的 XML 补全默认值
    output_manager.write_input_cards()

    # 生成网格
    mesh_info = GeometryMeshBuilder.build(input_cards.geometry_mesh, output_manager.directory, rng)
    output_manager.record_mesh(mesh_info.path)

    # 创建公共有限元对象
    fem_manager = FemManager(mesh_info, input_cards.simulation.solver)

    # 创建配置属性对象
    properties_manager = PropertiesManager(input_cards, mesh_info, fem_manager.fem_system)

    # 求解器对象
    solver_manager = SolverManager(input_cards.simulation.solver)

    # 创建 C++ 输出对象, 供各问题记录求解信息并保存场数据
    output_manager.initialize_field_output(fem_manager.fem_system)

    # 按本次求解场创建对应问题, 热与放射性可分别或同时启用。
    problem_types = {"temperature": Thermal, "concentration": Radioactivity}
    problems = {field.name: problem_types[field.name](input_cards, mesh_info, fem_manager.fem_system, properties_manager.properties_system, solver_manager.solver_system, output_manager.field_output) for field in input_cards.simulation.problem.field}

    # 创建耦合求解对象
    coupling_manager = CouplingManager(input_cards.simulation, properties_manager, problems, output_manager)

    # 运行模拟
    coupling_manager.run()

    # 标记模拟完成
    output_manager.finish_simulation(coupling_manager.time, coupling_manager.cycle)

    print("开始后处理", flush=True)
    postprocess_manager = PostprocessManager(input_cards)
    postprocess_manager.run()
    print(f"后处理完成: {postprocess_manager.postprocess_directory}", flush=True)


if __name__ == "__main__":
    main(project_directory / "cases/flat_geometry_2d/case.xml")
