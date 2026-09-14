from pathlib import Path
import numpy as np

from trimuse.geometry.builder import GeometryMeshBuilder
from trimuse.input.loader import load_case

def main() -> None:
    case_path = Path(__file__).resolve().parent / "cases" / "test" / "case202609131.xml"
    case = load_case(case_path)
    if isinstance(case, Path):
        print(f"输入模板已生成: {case}, 请填写后将输入路径指向该文件再运行")
        return
    rng = np.random.default_rng(case.case_config.random_seed)
    output_root = Path(case.case_config.output.directory).expanduser()
    if not output_root.is_absolute():
        output_root = case.case_path.parent / output_root
    run_directory = output_root / f"seed_{case.case_config.random_seed}"
    mesh_path = GeometryMeshBuilder.build(case.geometry_mesh_config, run_directory / "mesh", rng)
    print(f"网格已生成: {mesh_path}")

if __name__ == "__main__":
    main()
