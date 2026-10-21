"""随机颗粒内热源验证: COMSOL、TriMUSE、COMSOL - TriMUSE 三联图。"""
from pathlib import Path
import sys

case_scripts = Path(__file__).resolve().parent
sys.path.insert(0, str(case_scripts))
from transient_half_heat_2d_comsol import compare_temperature_fields


if __name__ == "__main__":
    case = case_scripts / "transient_real_particle_source_2d"
    compare_temperature_fields(
        case / "transient_real_particle_source_2d.vtu",
        case / "output_transient_real_particle_source_2d/seed_202609134",
    )
