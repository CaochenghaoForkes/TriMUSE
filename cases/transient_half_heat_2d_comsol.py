"""COMSOL CSV 与 TriMUSE 二维温度场对比: 每帧三联图及固定色标 GIF。

CSV 不含连接关系, 用其散点的线性三角插值在 TriMUSE 截面节点上求值。
仅画两套数据共同覆盖的三角形, 不在域外使用最近邻外推。
差值定义为 TriMUSE - COMSOL, 单位 K, 不是方程求解残差。
"""

from pathlib import Path
import re
import sys

import numpy as np
from scipy.spatial import Delaunay
from vtkmodules.util.numpy_support import vtk_to_numpy

project_directory = Path(__file__).resolve().parents[1]
if str(project_directory) not in sys.path:
    sys.path.insert(0, str(project_directory))

from trimuse.postprocess.output_reader import OutputReader
from trimuse.postprocess.section_plotter import SectionPlotter
from trimuse.postprocess.boundary_geometry import BoundaryGeometry
from trimuse.postprocess.animation_writer import save_gif
from trimuse.postprocess.plot_style import plot_style, annotate_time, scientific_axes, BOUNDARY_COLORS


def read_comsol_temperature(csv_path: str | Path) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """返回坐标 (点数,2)、时间 (时刻数,) 和温度 (点数,时刻数), 使用原文件单位 m/K/s。"""
    with Path(csv_path).expanduser().open(encoding="utf-8-sig") as stream:
        for line in stream:
            if line.startswith("% R,Z,"):
                times = np.array([float(value) for value in re.findall(r"@ t=([^,\s]+)", line)])
                break
        data = np.loadtxt(stream, delimiter=",", ndmin=2)
    return data[:, :2], times, data[:, 2:]


def prepare_comparison_grid(section, comsol_coordinates: np.ndarray):
    """保留 TriMUSE 面片连接, 预先计算 COMSOL 插值节点和重心权重, 固定网格只做一次。"""
    from matplotlib.tri import Triangulation

    coordinates = vtk_to_numpy(section.GetPoints().GetData())[:, :2]
    triangles = vtk_to_numpy(section.GetPolys().GetData()).reshape(-1, 4)[:, 1:]
    comsol_mesh = Delaunay(comsol_coordinates)
    simplex = comsol_mesh.find_simplex(coordinates)
    triangles = triangles[np.all(simplex[triangles] >= 0, axis=1)]
    if len(triangles) == 0:
        raise ValueError("COMSOL 与 TriMUSE 数据没有共同的二维覆盖范围")
    point_ids, connectivity = np.unique(triangles, return_inverse=True)
    positions = coordinates[point_ids]
    simplex = simplex[point_ids]
    transform = comsol_mesh.transform[simplex]
    first_weights = np.einsum("nij,nj->ni", transform[:, :2], positions - transform[:, 2])
    weights = np.column_stack((first_weights, 1.0 - first_weights.sum(axis=1)))
    source_ids = comsol_mesh.simplices[simplex]
    triangulation = Triangulation(positions[:, 0], positions[:, 1], connectivity.reshape(-1, 3))
    print(f"共同覆盖范围: 保留 {len(triangles)} 个三角形、{len(point_ids)} 个节点", flush=True)
    return triangulation, point_ids, source_ids, weights


@plot_style
def plot_comparison(triangulation, comsol_values: np.ndarray, trimuse_values: np.ndarray,
                    time: float, temperature_limits: tuple[float, float], error_limit: float,
                    boundaries: list, path: Path) -> Path:
    """三图共用空间范围; 温度共用色标, 温度差共用全时段正负对称色标。"""
    from matplotlib.figure import Figure
    from matplotlib.backends.backend_agg import FigureCanvasAgg
    from matplotlib.collections import LineCollection
    from matplotlib.lines import Line2D
    from matplotlib.colors import Normalize
    from mpl_toolkits.axes_grid1 import make_axes_locatable

    figure = Figure(figsize=(15, 7))
    FigureCanvasAgg(figure)
    axes = figure.subplots(1, 3, sharex=True, sharey=True)
    data = (comsol_values, trimuse_values, trimuse_values - comsol_values)
    titles = ("COMSOL", "TriMUSE", "TriMUSE - COMSOL")
    xmin, xmax = triangulation.x.min(), triangulation.x.max()
    ymin, ymax = triangulation.y.min(), triangulation.y.max()
    for index, (axis, values, title) in enumerate(zip(axes, data, titles)):
        limits = temperature_limits if index < 2 else (-error_limit, error_limit)
        image = axis.tripcolor(triangulation, values, shading="gouraud",
                               cmap="jet" if index < 2 else "PuOr_r", norm=Normalize(*limits))
        colorbar_axis = make_axes_locatable(axis).append_axes("right", size="5%", pad=0.10)
        colorbar = figure.colorbar(image, cax=colorbar_axis)
        colorbar.set_label("Temperature (K)" if index < 2 else "ΔT (K)", fontstyle="italic", fontsize=12)
        colorbar.ax.tick_params(labelsize=10)
        handles = []
        for boundary_index, (name, segments) in enumerate(boundaries):
            color = BOUNDARY_COLORS[boundary_index % len(BOUNDARY_COLORS)]
            axis.add_collection(LineCollection([segment[:, :2] for segment in segments], colors=color, linewidths=0.7))
            handles.append(Line2D([], [], color=color, label=name, linewidth=1))
        axis.set_xlim(xmin - 0.05 * (xmax - xmin), xmax + 0.40 * (xmax - xmin))
        axis.set_ylim(ymin - 0.12 * (ymax - ymin), ymax + 0.04 * (ymax - ymin))
        axis.set_aspect("equal")
        axis.set_title(title, fontsize=14)
        axis.set_xlabel("u (m)", fontstyle="italic")
        if index == 0:
            axis.set_ylabel("v (m)", fontstyle="italic")
        if handles:
            axis.legend(handles=handles, loc="lower left", ncols=2, fontsize=6, frameon=False)
        annotate_time(axis, time, position=(0.96, 0.96), fontsize=10)
        scientific_axes(axis)
        axis.tick_params(labelsize=10)
    figure.tight_layout()
    figure.savefig(path, dpi=150)
    return path


def compare_temperature_fields(csv_path: str | Path, trimuse_directory: str | Path,
                               output_directory: str | Path | None = None,
                               times: list[float] | None = None, duration: int = 120) -> Path:
    """trimuse_directory 为同一二维算例的 seed 输出根目录; 默认处理全部共同时间。

    times 可只选择几个真实时刻试画; 不按文件编号推算时间, 不做时间插值。
    返回 comparison.gif 路径, PNG 同目录保存。完整运行会遍历数据两次以固定色标。
    """
    csv_path = Path(csv_path).expanduser().resolve()
    trimuse_directory = Path(trimuse_directory).expanduser().resolve()
    destination = Path(output_directory).expanduser().resolve() if output_directory is not None else trimuse_directory / "postprocess/comsol_comparison"
    coordinates, comsol_times, temperatures = read_comsol_temperature(csv_path)
    reader = OutputReader(trimuse_directory, ["temperature"])
    frames = []
    for index, (time, frame_path) in enumerate(reader.series["temperature"]):
        match = np.flatnonzero(np.isclose(comsol_times, time, rtol=0, atol=1e-10))
        if len(match) and (times is None or any(np.isclose(time, selected, rtol=0, atol=1e-10) for selected in times)):
            frames.append((index, int(match[0]), time, frame_path.parent.name))
    if not frames:
        raise ValueError("没有找到双方共有的输出时刻")
    center, normal = (0.0, 0.0, 0.0), (0.0, 0.0, 1.0)
    first_mesh = reader.read_frame("temperature", frames[0][0])
    if first_mesh.GetCell(0).GetCellDimension() != 2:
        raise ValueError("本方法用于二维半球加热算例, 请传入对应的二维输出根目录")
    section = SectionPlotter.sample_section(first_mesh, center, normal)
    triangulation, point_ids, source_ids, weights = prepare_comparison_grid(section, coordinates)
    mesh_path = next((trimuse_directory / "mesh").glob("*.vtk"))
    boundaries = BoundaryGeometry(mesh_path, 2).section(center, normal)

    def frame_values(frame_index, time_index):
        mesh = reader.read_frame("temperature", frame_index)
        section = SectionPlotter.sample_section(mesh, center, normal)
        trimuse_values = vtk_to_numpy(section.GetPointData().GetArray("temperature"))[point_ids]
        comsol_values = (temperatures[source_ids, time_index] * weights).sum(axis=1)
        return comsol_values, trimuse_values

    lower, upper, maximum_error = np.inf, -np.inf, 0.0
    for index, time_index, _, _ in frames:
        comsol_values, trimuse_values = frame_values(index, time_index)
        lower = min(lower, comsol_values.min(), trimuse_values.min())
        upper = max(upper, comsol_values.max(), trimuse_values.max())
        maximum_error = max(maximum_error, np.abs(trimuse_values - comsol_values).max())
    if upper == lower:
        lower, upper = lower - 0.5, upper + 0.5
    maximum_error = maximum_error or 1e-12
    destination.mkdir(parents=True, exist_ok=True)
    paths = []
    print(f"温度色标: {lower:.6g} ~ {upper:.6g} K; 温度差色标: ±{maximum_error:.6g} K", flush=True)
    for count, (index, time_index, time, name) in enumerate(frames, start=1):
        comsol_values, trimuse_values = frame_values(index, time_index)
        paths.append(plot_comparison(triangulation, comsol_values, trimuse_values, time,
                                     (lower, upper), maximum_error, boundaries, destination / f"{name}.png"))
        print(f"对比绘图 {count}/{len(frames)}, t={time:.6g} s", flush=True)
    return save_gif(paths, destination / "comparison.gif", duration=duration)


if __name__ == "__main__":
    case_directory = project_directory / "cases/transient_half_heat_2d"
    compare_temperature_fields(
        case_directory / "transient_half_heat_2d_comsol.csv",
        case_directory / "output_transient_half_heat_2d/seed_202609134",
    )
