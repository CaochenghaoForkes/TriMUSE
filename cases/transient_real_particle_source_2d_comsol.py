"""随机五层颗粒核心内热源算例的 COMSOL/单次求解/Picard 五联图及 GIF。

两幅温差分别为 COMSOL - 单次求解、COMSOL - Picard (K)。
"""

from pathlib import Path
import sys

case_scripts = Path(__file__).resolve().parent
if str(case_scripts) not in sys.path:
    sys.path.insert(0, str(case_scripts))

from transient_real_particle_heat_2d_comsol import compare_temperature_fields as _compare_temperature_fields


def compare_temperature_fields(csv_path: str | Path, single_solve_directory: str | Path,
                               picard_directory: str | Path,
                               output_directory: str | Path | None = None,
                               times: list[float] | None = None, duration: int = 120) -> Path:
    """生成 COMSOL、单次求解、Picard、COMSOL−单次、COMSOL−Picard 五联图。

    times 缺省时处理全部共同时间; 温度使用 jet, 温差使用黄紫色标。
    默认保存至 Picard seed/postprocess/comsol_picard_comparison/。
    """
    return _compare_temperature_fields(csv_path, single_solve_directory, picard_directory, output_directory,
                                       times=times, duration=duration)


if __name__ == "__main__":
    case_directory = case_scripts / "transient_real_particle_source_2d"
    compare_temperature_fields(
        case_directory / "transient_real_particle_source_2d.csv",
        case_directory / "output_transient_real_particle_source_2d_max_iteration_1/seed_202609134",
        case_directory / "output_transient_real_particle_source_2d/seed_202609134",
    )
