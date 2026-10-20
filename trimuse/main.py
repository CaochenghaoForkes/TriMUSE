from pathlib import Path
import sys

project_directory = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(project_directory))
sys.path.insert(0, str(project_directory / "build" / "bindings"))
from trimuse.input import load_input_cards
from trimuse.output import OutputManager

if __name__ == "__main__":

    # 读取输入
    chief_path: Path = project_directory / "cases/steady_constant_power_ideal_sphere_1d/case.xml"
    input_cards = load_input_cards(chief_path)

    output_manager = OutputManager(input_cards)
    output_directory = output_manager.directory

    if output_manager.is_complete():
        print(f"模拟已完成, 进入后处理", flush=True)
    else:
        # 仅在需要计算时加载内核与几何模块。
        import numpy as np
        from trimuse.geometry.builder import GeometryMeshBuilder
        from trimuse.fem import FemManager
        from trimuse.properties import PropertiesManager
        from trimuse.solver import SolverManager
        from trimuse.problem import Thermal
        from trimuse.coupling import CouplingManager

        rng = np.random.default_rng(input_cards.chief.random_seed)

        output_manager.write_input_cards()

        # 生成或复用网格, 创建公共有限元对象。
        mesh_info = GeometryMeshBuilder.build(input_cards.geometry_mesh, output_directory, rng)
        output_manager.record_mesh(mesh_info.path)
        fem_manager = FemManager(mesh_info, input_cards.simulation.solver)

        # 配置属性、求解器和热问题, 同时初始化温度场。
        properties_manager = PropertiesManager(input_cards, mesh_info, fem_manager.fem_system)
        solver_manager = SolverManager(input_cards.simulation.solver)
        thermal_problem = Thermal(input_cards, mesh_info, fem_manager.fem_system, properties_manager.properties_system, solver_manager.solver_system)

        # 推进全部时间步并保存结果; 只有正常结束后才标记完成。
        output_manager.initialize_field_output(fem_manager.fem_system)
        coupling_manager = CouplingManager(input_cards.simulation, properties_manager, {"thermal": thermal_problem}, output_manager)
        coupling_manager.run()
        output_manager.finish_simulation(coupling_manager.time, coupling_manager.cycle)

    # 新计算及已有完整结果均进入后处理, 不依赖本次创建求解对象。
    from trimuse.postprocess.postprocess import PostprocessManager

    print("开始后处理", flush=True)
    postprocess_manager = PostprocessManager(input_cards)
    postprocess_manager.run()
    print(f"后处理完成: {postprocess_manager.postprocess_directory}", flush=True)
