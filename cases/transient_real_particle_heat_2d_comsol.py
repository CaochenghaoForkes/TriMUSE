"""随机五层颗粒加热验证: COMSOL、单次求解、Picard 及两幅温差五联图。"""
from pathlib import Path
import sys

case_scripts = Path(__file__).resolve().parent
sys.path.insert(0, str(case_scripts))
from transient_half_heat_2d_comsol import compare_temperature_fields as _compare


def compare_temperature_fields(vtu_path: str | Path, single_solve_directory: str | Path,
                               picard_directory: str | Path, output_directory: str | Path | None = None,
                               times: list[float] | None = None, duration: int = 120) -> Path:
    return _compare(vtu_path, picard_directory, output_directory, times, duration,
                    single_solve_directory=single_solve_directory)


if __name__ == "__main__":
    case = case_scripts / "transient_real_particle_heat_2d"
    compare_temperature_fields(
        case / "transient_real_particle_heat_2d.vtu",
        case / "output_transient_real_particle_heat_2d_max_iteration_1/seed_202609134",
        case / "output_transient_real_particle_heat_2d/seed_202609134",
    )
