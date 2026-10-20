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

    # 创建输出对象
    output_manager = OutputManager(input_cards)

    if output_manager.is_complete():
        print(f"模拟已完成, 进入后处理", flush=True)
    else:
        import numpy as np
        from trimuse.geometry.builder import GeometryMeshBuilder
        from trimuse.fem import FemManager
        from trimuse.properties import PropertiesManager
        from trimuse.solver import SolverManager
        from trimuse.problem import Thermal
        from trimuse.coupling import CouplingManager

        # 创建随机数生成器
        rng = np.random.default_rng(input_cards.chief.random_seed)

        # 保存可重新读取的 XML 补全默认值
        output_manager.write_input_cards()

        # 生成或复用网格
        mesh_info = GeometryMeshBuilder.build(input_cards.geometry_mesh, output_manager.directory, rng)
        output_manager.record_mesh(mesh_info.path)

        # 创建公共有限元对象
        fem_manager = FemManager(mesh_info, input_cards.simulation.solver)

        # 创建配置属性对象
        properties_manager = PropertiesManager(input_cards, mesh_info, fem_manager.fem_system)

        # 求解器对象
        solver_manager = SolverManager(input_cards.simulation.solver)

        # 热问题对象
        thermal_problem = Thermal(input_cards, mesh_info, fem_manager.fem_system, properties_manager.properties_system, solver_manager.solver_system)

        # 推进全部时间步并保存结果; 只有正常结束后才标记完成
        output_manager.initialize_field_output(fem_manager.fem_system)

        # 创建耦合求解对象
        coupling_manager = CouplingManager(input_cards.simulation, properties_manager, {"thermal": thermal_problem}, output_manager)

        # 运行模拟
        coupling_manager.run()

        # 标记模拟完成
        output_manager.finish_simulation(coupling_manager.time, coupling_manager.cycle)

    # 新计算及已有完整结果均进入后处理, 不依赖本次创建求解对象
    from trimuse.postprocess.postprocess import PostprocessManager

    print("开始后处理", flush=True)
    postprocess_manager = PostprocessManager(input_cards)
    postprocess_manager.run()
    print(f"后处理完成: {postprocess_manager.postprocess_directory}", flush=True)
