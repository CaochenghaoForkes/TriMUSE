from pathlib import Path
import sys

project_directory = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(project_directory))
sys.path.insert(0, str(project_directory / "build" / "bindings"))
from trimuse.input import load_input_cards

if __name__ == "__main__":

    # 读取输入
    chief_path = project_directory / "cases/source/case.xml"
    input_cards = load_input_cards(chief_path)

    output_directory = Path(input_cards.chief.output.directory)
    completed = output_directory / "simulation_complete.flag"

    if completed.is_file():
        print(f"模拟已完成, 进入后处理", flush=True)
    else:
        # 仅在需要计算时加载内核与几何模块。
        import numpy as np
        from _core import OutputManager
        from trimuse.geometry.builder import GeometryMeshBuilder
        from trimuse.fem import FemManager
        from trimuse.properties import PropertiesManager
        from trimuse.solver import SolverManager
        from trimuse.problem import Thermal
        from trimuse.coupling import CouplingManager

        # 清除失效标记; 中断或报错的计算不会留下完成标记。
        completed.unlink(missing_ok=True)
        rng = np.random.default_rng(input_cards.chief.random_seed)

        # 生成或复用网格, 创建公共有限元对象。
        mesh_info = GeometryMeshBuilder.build(input_cards.geometry_mesh, output_directory, rng)
        fem_manager = FemManager(mesh_info, input_cards.simulation.solver)

        # 配置属性、求解器和热问题, 同时初始化温度场。
        properties_manager = PropertiesManager(input_cards, mesh_info, fem_manager.fem_system)
        solver_manager = SolverManager(input_cards.simulation.solver)
        thermal_problem = Thermal(input_cards, mesh_info, fem_manager.fem_system, properties_manager.properties_system, solver_manager.solver_system)

        # 推进全部时间步并保存结果; 只有正常结束后才标记完成。
        output_manager = OutputManager(fem_manager.fem_system, str(output_directory))
        coupling_manager = CouplingManager(input_cards.simulation, properties_manager, {"thermal": thermal_problem}, output_manager)
        coupling_manager.run()
        completed.touch()

    # 后续在这里根据 input_cards.postprocess 提取已有结果, 不依赖本次创建求解对象。
    print("后处理阶段: 结果已就绪, 数据提取与绘图接口待接入", flush=True)
