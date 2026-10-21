"""三维局部冷热验证: 按 postprocess.xml 中的截面生成三联图, 不再画外表面。"""
from pathlib import Path
from math import sin, cos, degrees
import argparse
import sys

case_scripts = Path(__file__).resolve().parent
sys.path.insert(0, str(case_scripts))
from transient_half_heat_2d_comsol import compare_temperature_fields
from trimuse.input.postprocess.config import PostprocessCardLoader


def compare_sections(vtu_path: str | Path, seed_directory: str | Path,
                     postprocess_card: str | Path, output_directory: str | Path | None = None,
                     times: list[float] | None = None, duration: int = 120) -> list[Path]:
    """使用三维 VTU 原单元在 TriMUSE 截面节点求值, 每个截面一套 PNG/GIF。"""
    destination = Path(output_directory) if output_directory else Path(seed_directory) / "postprocess/comsol_comparison"
    config = PostprocessCardLoader.load(postprocess_card)
    results = []
    for section in config.sections:
        center = (section.center.x, section.center.y, section.center.z)
        theta, phi = section.normal.theta, section.normal.phi
        normal = (sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta))
        annotation = [rf"$\theta={degrees(theta):.4g}^\circ$", rf"$\varphi={degrees(phi):.4g}^\circ$",
                      *["$" + axis + rf"={value:.4g}\ \mathit{{m}}$" for axis, value in zip("xyz", center)]]
        results.append(compare_temperature_fields(
            vtu_path, seed_directory, destination / section.name, times, duration,
            center=center, normal=normal, annotation=annotation))
    return results


if __name__ == "__main__":
    case = case_scripts / "transient_local_heat_cool_3d"
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vtu", type=Path, default=case / "transient_local_heat_cool_3d_comsol.vtu")
    parser.add_argument("--seed", type=Path, default=case / "output_transient_local_heat_cool_3d/seed_202609134")
    parser.add_argument("--postprocess", type=Path, default=case / "postprocess.xml")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--times", type=float, nargs="+")
    args = parser.parse_args()
    for path in compare_sections(args.vtu, args.seed, args.postprocess, args.output, args.times):
        print(path)
