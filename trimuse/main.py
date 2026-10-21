from __future__ import annotations

from pathlib import Path
import argparse
import sys
from typing import TYPE_CHECKING

project_directory = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(project_directory))
sys.path.insert(0, str(project_directory / "build" / "bindings"))
from trimuse.input import load_input_cards
from trimuse.output import OutputManager

if TYPE_CHECKING:
    from trimuse.input.chief.config import ChiefCard


def output_directory_state(directory: Path) -> str:
    """区分空目录、已有完整结果和其他非空目录, 不创建或修改文件。"""
    if not directory.exists():
        return "available"
    if not directory.is_dir():
        raise NotADirectoryError(f"输出路径不是目录: {directory}")
    if not any(directory.iterdir()):
        return "available"
    return "complete" if OutputManager.is_complete(directory) else "occupied"


def prompt_choice(prompt: str, choices: set[str]) -> str:
    while True:
        try:
            choice = input(prompt).strip()
        except EOFError as error:
            raise RuntimeError("当前环境不能交互, 请设置 workflow.mode 或 --mode 为 run/postprocess") from error
        if choice in choices:
            return choice
        print(f"请输入: {', '.join(sorted(choices))}")


def prompt_new_output_directory(directory: Path, base_directory: Path, seed: int) -> Path:
    """选择新的用户根目录, 保持 <用户根目录>/seed_<种子> 的固定结构。"""
    root = directory.parent
    suffix = 2
    suggestion = root.with_name(f"{root.name}_{suffix}")
    while suggestion.exists():
        suffix += 1
        suggestion = root.with_name(f"{root.name}_{suffix}")
    while True:
        try:
            value = input(f"新的输出根目录 (自动追加 seed_{seed})\n回车使用: {suggestion}\n> ").strip()
        except EOFError as error:
            raise RuntimeError("当前环境不能交互, 请修改总调度卡的 output.directory 后使用 --mode run") from error
        candidate = Path(value).expanduser() if value else suggestion
        if not candidate.is_absolute():
            candidate = base_directory / candidate
        candidate = candidate.resolve()
        if candidate.exists() and not candidate.is_dir():
            print(f"输出根路径不是目录, 请重新输入: {candidate}")
            continue
        candidate = candidate / f"seed_{seed}"
        if output_directory_state(candidate) == "available":
            return candidate
        print(f"目录已被占用, 请重新输入: {candidate}")


def resolve_workflow(chief: ChiefCard, base_directory: Path) -> str | None:
    """返回 run/postprocess 或取消; 重算使用新目录, 不覆盖原结果。"""
    mode = chief.workflow.mode
    directory = Path(chief.output.directory)
    state = output_directory_state(directory)
    if mode == "run":
        if state != "available":
            raise FileExistsError(f"输出目录已被占用: {directory}; 请修改 output.directory 或使用 ask 模式")
        return "run"
    if mode == "postprocess":
        if state != "complete":
            raise FileNotFoundError(f"后处理需要 simulation_report.txt 最后一行为 simulation_completed: {directory}")
        return "postprocess"
    if state == "available":
        return "run"
    if state == "complete":
        print(f"已有完整模拟结果: {directory}\n[1] 使用新目录重新计算并后处理\n[2] 复用已有结果, 只后处理\n[3] 取消")
        choice = prompt_choice("请选择: ", {"1", "2", "3"})
        if choice == "2":
            return "postprocess"
        if choice == "3":
            return None
    else:
        print(f"输出目录非空, 但模拟未完成: {directory}\n[1] 使用新目录重新计算并后处理\n[2] 取消")
        if prompt_choice("请选择: ", {"1", "2"}) == "2":
            return None
    chief.output.directory = str(prompt_new_output_directory(directory, base_directory, chief.random_seed))
    return "run"


def main(argv: list[str] | None = None) -> None:
    parser = argparse.ArgumentParser(description="TriMUSE: 指定总调度 XML 卡运行模拟与后处理")
    parser.add_argument("chief_path", nargs="?", type=Path, help="总调度卡路径, 相对路径以当前工作目录为基准")
    parser.add_argument("-i", "--input", dest="input_path", type=Path, help="总调度卡路径, 与位置参数二选一")
    parser.add_argument("--mode", choices=("ask", "run", "postprocess"), help="覆盖总调度卡的 workflow.mode")
    args = parser.parse_args(argv)
    if args.chief_path is not None and args.input_path is not None:
        parser.error("请使用位置参数或 -i/--input 指定输入卡, 不要同时使用")

    # 读取输入
    chief_path = (args.input_path or args.chief_path or project_directory / "cases/transient_real_particle_source_2d/case.xml").expanduser().resolve()
    input_cards = load_input_cards(chief_path)
    if args.mode is not None:
        input_cards.chief.workflow.mode = args.mode
    workflow = resolve_workflow(input_cards.chief, chief_path.parent)
    if workflow is None:
        print("已取消。")
        return

    # 创建输出对象
    output_manager = OutputManager(input_cards)

    if workflow == "postprocess":
        print("复用已有完整结果, 进入后处理", flush=True)
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

        # 创建 C++ 输出对象, 供热问题记录求解信息并保存场数据
        output_manager.initialize_field_output(fem_manager.fem_system)

        # 热问题对象
        thermal_problem = Thermal(input_cards, mesh_info, fem_manager.fem_system, properties_manager.properties_system, solver_manager.solver_system, output_manager.field_output)

        # 创建耦合求解对象
        coupling_manager = CouplingManager(input_cards.simulation, properties_manager, {thermal_problem.field_state: thermal_problem}, output_manager)

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


if __name__ == "__main__":
    main()
