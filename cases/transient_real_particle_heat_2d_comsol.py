"""随机五层颗粒 COMSOL、单次求解、完整 Picard 的五联图及 GIF。

复用半球算例的 CSV 读取、共同网格插值、固定色标和动画实现。
两幅差值依次为 COMSOL - 单次求解、COMSOL - Picard (K)。
CSV 插值本身也会引入误差; 单次求解指每步只做一次物性冻结的线性求解。
"""

from pathlib import Path
import sys
import numpy as np
from vtkmodules.util.numpy_support import vtk_to_numpy

case_scripts = Path(__file__).resolve().parent
if str(case_scripts) not in sys.path:
    sys.path.insert(0, str(case_scripts))

from transient_half_heat_2d_comsol import (
    read_comsol_temperature, prepare_comparison_grid, plot_comparison,
    OutputReader, SectionPlotter, BoundaryGeometry, save_gif,
)


def compare_temperature_fields(csv_path: str | Path, single_solve_directory: str | Path,
                               picard_directory: str | Path,
                               output_directory: str | Path | None = None,
                               times: list[float] | None = None, duration: int = 120) -> Path:
    """输入 CSV、单次求解和完整 Picard 的两个 seed 根目录, 仅比较三者共同时间。

    前三图共用全时段温度色标; 后两图共用正负对称温差色标。
    差值图标注各自截至当前帧的最大绝对温差, 历史范围为本次选中的共同输出时刻。
    PNG/GIF 默认保存至 Picard seed/postprocess/comsol_picard_comparison/。
    """
    picard_directory = Path(picard_directory).expanduser().resolve()
    destination = Path(output_directory).expanduser().resolve() if output_directory is not None else picard_directory / "postprocess/comsol_picard_comparison"
    coordinates, comsol_times, temperatures = read_comsol_temperature(csv_path)
    single_reader = OutputReader(single_solve_directory, ["temperature"])
    picard_reader = OutputReader(picard_directory, ["temperature"])
    single_times = np.asarray(single_reader.times("temperature"))
    frames = []
    for picard_index, (time, frame_path) in enumerate(picard_reader.series["temperature"]):
        comsol_match = np.flatnonzero(np.isclose(comsol_times, time, rtol=0, atol=1e-10))
        single_match = np.flatnonzero(np.isclose(single_times, time, rtol=0, atol=1e-10))
        if len(comsol_match) and len(single_match) and (times is None or any(np.isclose(time, selected, rtol=0, atol=1e-10) for selected in times)):
            frames.append((picard_index, int(single_match[0]), int(comsol_match[0]), time, frame_path.parent.name))
    if not frames:
        raise ValueError("没有找到 COMSOL、单次求解和 Picard 三者共同的时刻")

    center, normal = (0.0, 0.0, 0.0), (0.0, 0.0, 1.0)
    def read_section(reader, index):
        return SectionPlotter.sample_section(reader.read_frame("temperature", index), center, normal)

    first_picard = read_section(picard_reader, frames[0][0])
    first_single = read_section(single_reader, frames[0][1])
    picard_positions = vtk_to_numpy(first_picard.GetPoints().GetData())
    single_positions = vtk_to_numpy(first_single.GetPoints().GetData())
    # 两次 TriMUSE 实验必须使用同一网格及节点编号, 不能仅按数组下标比较不同网格。
    if picard_positions.shape != single_positions.shape or not np.allclose(picard_positions, single_positions, rtol=0, atol=1e-14):
        raise ValueError("单次求解与 Picard 的输出网格不一致, 请使用同一网格进行对照")
    triangulation, point_ids, source_ids, weights = prepare_comparison_grid(first_picard, coordinates)
    mesh_path = next((picard_directory / "mesh").glob("*.vtk"))
    boundaries = BoundaryGeometry(mesh_path, 2).section(center, normal)

    def frame_values(picard_index, single_index, comsol_index):
        picard = read_section(picard_reader, picard_index)
        single = read_section(single_reader, single_index)
        picard_values = vtk_to_numpy(picard.GetPointData().GetArray("temperature"))[point_ids].copy()
        single_values = vtk_to_numpy(single.GetPointData().GetArray("temperature"))[point_ids].copy()
        comsol_values = (temperatures[source_ids, comsol_index] * weights).sum(axis=1)
        return comsol_values, single_values, picard_values

    lower, upper, maximum_error = np.inf, -np.inf, 0.0
    for picard_index, single_index, comsol_index, _, _ in frames:
        comsol_values, single_values, picard_values = frame_values(picard_index, single_index, comsol_index)
        lower = min(lower, comsol_values.min(), single_values.min(), picard_values.min())
        upper = max(upper, comsol_values.max(), single_values.max(), picard_values.max())
        maximum_error = max(maximum_error, np.abs(comsol_values - single_values).max(),
                            np.abs(comsol_values - picard_values).max())
    if upper == lower:
        lower, upper = lower - 0.5, upper + 0.5
    maximum_error = maximum_error or 1e-12
    destination.mkdir(parents=True, exist_ok=True)
    paths = []
    print(f"温度共用色标: {lower:.6g} ~ {upper:.6g} K; 两幅温差共用色标: ±{maximum_error:.6g} K", flush=True)
    historical_errors = np.zeros(2)
    for count, (picard_index, single_index, comsol_index, time, name) in enumerate(frames, start=1):
        comsol_values, single_values, picard_values = frame_values(picard_index, single_index, comsol_index)
        historical_errors = np.maximum(historical_errors, [np.abs(comsol_values - single_values).max(),
                                                          np.abs(comsol_values - picard_values).max()])
        paths.append(plot_comparison(triangulation, comsol_values, picard_values, time,
                                     (lower, upper), maximum_error, boundaries, destination / f"{name}.png",
                                     single_values=single_values, historical_errors=tuple(historical_errors)))
        print(f"五联图 {count}/{len(frames)}, t={time:.6g} s", flush=True)
    return save_gif(paths, destination / "comparison.gif", duration=duration)


if __name__ == "__main__":
    case_directory = case_scripts / "transient_real_particle_heat_2d"
    compare_temperature_fields(
        case_directory / "transient_real_particle_heat_2d.csv",
        case_directory / "output_transient_real_particle_heat_2d_max_iteration_1/seed_202609134",
        case_directory / "output_transient_real_particle_heat_2d/seed_202609134",
    )
