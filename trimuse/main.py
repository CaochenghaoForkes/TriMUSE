from __future__ import annotations
import sys
from pathlib import Path

# 直接运行此文件时，使项目包和编译后的 C++ 扩展可导入
project_directory = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(project_directory))
sys.path.insert(0, str(project_directory / "build" / "bindings"))

from trimuse.coupling import CouplingManager
from trimuse.fem import FemManager
from trimuse.geometry.builder import GeometryMeshBuilder
from trimuse.input import load_input_cards
from trimuse.output import OutputManager
from trimuse.postprocess.postprocess import PostprocessManager
from trimuse.properties import PropertiesManager
from trimuse.solver import SolverManager

STARTUP_BANNER = r"""
 _____          _   __  __   _   _   ____    _____
|_   _|  _ __  (_) |  \/  | | | | | / ___|  | ____|
  | |   | '__| | | | |\/| | | | | | \___ \  |  _|
  | |   | |    | | | |  | | | |_| |  ___) | | |___
  |_|   |_|    |_| |_|  |_|  \___/  |____/  |_____|

    TRISO Multiphysics Simulation Environment
"""

def main(chief_path: str | Path) -> None:
    """读取总调度卡，依次执行网格生成、模拟与后处理"""
    print(STARTUP_BANNER, flush=True)

    # 读取输入，准备输出目录并生成网格
    input_cards = load_input_cards(Path(chief_path))
    output_manager = OutputManager(input_cards)
    mesh_info = GeometryMeshBuilder.build_case(input_cards, output_manager.directory)

    # 初始化各求解组件
    fem_manager = FemManager(input_cards, mesh_info)
    properties_manager = PropertiesManager(input_cards, fem_manager)
    solver_manager = SolverManager(input_cards)
    coupling_manager = CouplingManager(input_cards, fem_manager, properties_manager, solver_manager, output_manager)

    # 运行计算和后处理
    coupling_manager.run()
    PostprocessManager(input_cards).run()


if __name__ == "__main__":
    main(project_directory / "cases/test/case.xml")

