"""COMSOL VTU 与 TriMUSE 温度截面对比, 三联/五联图及固定色标 GIF。

VTU 中每个时刻是一个温度数组; 使用其真实单元形函数在 TriMUSE 截面节点求值。
空间插值权重只初始化一次, 不重新构造 Delaunay 网格、不在域外外推。
所有差值统一为 COMSOL - TriMUSE (K), 不是方程求解残差。
"""

from pathlib import Path
import re
import sys

import numpy as np
from scipy.sparse import csr_matrix
from vtkmodules.util.numpy_support import vtk_to_numpy
from vtkmodules.vtkCommonCore import reference
from vtkmodules.vtkCommonDataModel import vtkStaticCellLocator, vtkGenericCell
from vtkmodules.vtkIOXML import vtkXMLUnstructuredGridReader

project_directory = Path(__file__).resolve().parents[1]
if str(project_directory) not in sys.path:
    sys.path.insert(0, str(project_directory))

from trimuse.postprocess.output_reader import OutputReader
from trimuse.postprocess.section_plotter import SectionPlotter
from trimuse.postprocess.boundary_geometry import BoundaryGeometry
from trimuse.postprocess.animation_writer import save_gif
from trimuse.postprocess.plot_style import plot_style, annotate_time, scientific_axes, BOUNDARY_COLORS


class ComsolReader:
    """读取一个含完整时间序列的 COMSOL VTU, 中文/英文温度数组名均可。"""

    def __init__(self, path: str | Path) -> None:
        reader = vtkXMLUnstructuredGridReader()
        reader.SetFileName(str(Path(path).expanduser().resolve()))
        reader.Update()
        self.mesh = reader.GetOutput()
        self.arrays = []
        data = self.mesh.GetPointData()
        for index in range(data.GetNumberOfArrays()):
            name = data.GetArrayName(index)
            match = re.search(r"@\s*t\s*=\s*([+\-\d.eE]+)", name.replace("_", " "))
            if match and data.GetArray(index).GetNumberOfComponents() == 1:
                self.arrays.append((float(match[1]), name))
        self.arrays.sort()
        if not self.arrays:
            raise ValueError("VTU 中没有找到标有 @ t=... 的标量温度数组; 请仅导出温度 T")
        self.times = np.array([time for time, _ in self.arrays])
        print(f"COMSOL: {self.mesh.GetNumberOfPoints()} 点, {self.mesh.GetNumberOfCells()} 单元, {len(self.times)} 时刻", flush=True)

    def values(self, index: int) -> np.ndarray:
        return vtk_to_numpy(self.mesh.GetPointData().GetArray(self.arrays[index][1]))


def section_basis(normal):
    normal = np.asarray(normal, dtype=float)
    normal /= np.linalg.norm(normal)
    axis = np.eye(3)[np.argmin(np.abs(normal))]
    first = axis - np.dot(axis, normal) * normal
    first /= np.linalg.norm(first)
    return first, np.cross(normal, first)


def prepare_comparison_grid(section, comsol_mesh, center, normal):
    """缓存原单元插值权重; 仅保留三个顶点都在 COMSOL 域内的截面三角形。"""
    from matplotlib.tri import Triangulation

    coordinates = vtk_to_numpy(section.GetPoints().GetData())
    triangles = vtk_to_numpy(section.GetPolys().GetData()).reshape(-1, 4)[:, 1:]
    locator = vtkStaticCellLocator()
    locator.SetDataSet(comsol_mesh)
    locator.BuildLocator()
    cell = vtkGenericCell()
    weights = [0.0] * comsol_mesh.GetMaxCellSize()
    sub_id, local = reference(0), [0.0, 0.0, 0.0]
    # 容许两套导出文件的坐标舍入差, 不对真实域外点做最近邻外推。
    tolerance = 1e-7 * comsol_mesh.GetLength()
    inside = np.zeros(len(coordinates), dtype=bool)
    rows, columns, coefficients = [], [], []
    for index, point in enumerate(coordinates):
        cell_id = locator.FindCell(point, tolerance ** 2, cell, sub_id, local, weights)
        if cell_id < 0:
            continue
        inside[index] = True
        for node in range(cell.GetNumberOfPoints()):
            rows.append(index)
            columns.append(cell.GetPointId(node))
            coefficients.append(weights[node])
    valid = np.all(inside[triangles], axis=1)
    retained = triangles[valid]
    if not len(retained):
        raise ValueError("COMSOL 网格未覆盖指定 TriMUSE 截面")
    point_ids, connectivity = np.unique(retained, return_inverse=True)
    interpolation = csr_matrix((coefficients, (rows, columns)),
                               shape=(len(coordinates), comsol_mesh.GetNumberOfPoints()))[point_ids]
    first, second = section_basis(normal)
    positions = coordinates[point_ids] - np.asarray(center)
    triangulation = Triangulation(positions @ first, positions @ second, connectivity.reshape(-1, 3))
    print(f"共同截面: {len(retained)}/{len(triangles)} 三角形, {len(point_ids)} 节点; "
          f"域外裁除 {np.count_nonzero(~valid)} 三角形", flush=True)
    return triangulation, point_ids, interpolation


@plot_style
def plot_comparison(triangulation, comsol_values: np.ndarray, trimuse_values: np.ndarray,
                    time: float, temperature_limits: tuple[float, float], error_limit: float,
                    boundaries: list, path: Path, single_values: np.ndarray | None = None,
                    historical_errors: tuple[float, ...] | None = None,
                    annotation: list[str] | None = None) -> Path:
    """默认三联图; 提供 single_values 时绘制单次求解/Picard 五联图。"""
    from matplotlib.figure import Figure
    from matplotlib.backends.backend_agg import FigureCanvasAgg
    from matplotlib.collections import LineCollection
    from matplotlib.lines import Line2D
    from matplotlib.colors import Normalize
    from mpl_toolkits.axes_grid1 import make_axes_locatable

    data = (comsol_values, trimuse_values, comsol_values - trimuse_values)
    titles = ("COMSOL", "TriMUSE", "COMSOL - TriMUSE")
    temperature_count = 2
    if single_values is not None:
        data = (comsol_values, single_values, trimuse_values,
                comsol_values - single_values, comsol_values - trimuse_values)
        titles = ("COMSOL", "TriMUSE (single solve)", "TriMUSE (Picard)",
                  "COMSOL - single solve", "COMSOL - Picard")
        temperature_count = 3
    figure = Figure(figsize=(5 * len(data), 7))
    FigureCanvasAgg(figure)
    axes = figure.subplots(1, len(data), sharex=True, sharey=True)
    xmin, xmax = triangulation.x.min(), triangulation.x.max()
    ymin, ymax = triangulation.y.min(), triangulation.y.max()
    for index, (axis, values, title) in enumerate(zip(axes, data, titles)):
        is_temperature = index < temperature_count
        limits = temperature_limits if is_temperature else (-error_limit, error_limit)
        image = axis.tripcolor(triangulation, values, shading="gouraud",
                               cmap="jet" if is_temperature else "PuOr_r", norm=Normalize(*limits))
        colorbar_axis = make_axes_locatable(axis).append_axes("right", size="5%", pad=0.10)
        colorbar = figure.colorbar(image, cax=colorbar_axis)
        colorbar.set_label("Temperature (K)" if is_temperature else "ΔT (K)", fontstyle="italic", fontsize=12)
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
        details = list(annotation or [])
        if not is_temperature and historical_errors is not None:
            error = historical_errors[index - temperature_count]
            coefficient, exponent = f"{error:.2e}".split("e")
            details.append(rf"$\max_{{\tau\leq t,\,\mathbf{{x}}}} |\Delta T| = {coefficient}\times10^{{{int(exponent)}}}\ \mathit{{K}}$")
        annotate_time(axis, time, position=(0.96, 0.96), details=details, fontsize=10)
        scientific_axes(axis)
        axis.tick_params(labelsize=10)
    figure.tight_layout()
    figure.savefig(path, dpi=150)
    return path


def compare_temperature_fields(vtu_path: str | Path, trimuse_directory: str | Path,
                               output_directory: str | Path | None = None,
                               times: list[float] | None = None, duration: int = 120, *,
                               single_solve_directory: str | Path | None = None,
                               center=(0.0, 0.0, 0.0), normal=(0.0, 0.0, 1.0),
                               annotation: list[str] | None = None) -> Path:
    """无 single_solve_directory 时三联图, 有则五联图; 仅比较共同真实时刻。

    温度及差值色标跨全部选中帧固定; 每幅差值分别记录选中历史帧的最大绝对温差。
    COMSOL 只从 VTU 原网格插值, 不做时间插值。固定网格的截面编号和权重跨帧复用。
    """
    trimuse_directory = Path(trimuse_directory).expanduser().resolve()
    folder = "comsol_picard_comparison" if single_solve_directory is not None else "comsol_comparison"
    destination = Path(output_directory).expanduser().resolve() if output_directory is not None else trimuse_directory / "postprocess" / folder
    comsol = ComsolReader(vtu_path)
    reader = OutputReader(trimuse_directory, ["temperature"])
    single_reader = OutputReader(single_solve_directory, ["temperature"]) if single_solve_directory is not None else None
    single_times = np.array(single_reader.times("temperature")) if single_reader else None
    frames = []
    for index, (time, frame_path) in enumerate(reader.series["temperature"]):
        match = np.flatnonzero(np.isclose(comsol.times, time, rtol=0, atol=1e-10))
        if not len(match) or times is not None and not any(np.isclose(time, t, rtol=0, atol=1e-10) for t in times):
            continue
        single_index = None
        if single_reader:
            single_match = np.flatnonzero(np.isclose(single_times, time, rtol=0, atol=1e-10))
            if not len(single_match):
                continue
            single_index = int(single_match[0])
        frames.append((index, int(match[0]), single_index, time, frame_path.parent.name))
    if not frames:
        raise ValueError("没有找到各方共有的输出时刻")

    first_mesh = reader.read_frame("temperature", frames[0][0])
    dimension = first_mesh.GetCell(0).GetCellDimension()
    section = SectionPlotter.sample_section(first_mesh, center, normal)
    triangulation, point_ids, interpolation = prepare_comparison_grid(section, comsol.mesh, center, normal)
    if single_reader:
        single = SectionPlotter.sample_section(single_reader.read_frame("temperature", frames[0][2]), center, normal)
        positions = vtk_to_numpy(section.GetPoints().GetData())
        single_positions = vtk_to_numpy(single.GetPoints().GetData())
        faces = vtk_to_numpy(section.GetPolys().GetData())
        if (positions.shape != single_positions.shape or not np.allclose(positions, single_positions, rtol=0, atol=1e-14)
                or not np.array_equal(faces, vtk_to_numpy(single.GetPolys().GetData()))):
            raise ValueError("单次求解与 Picard 必须使用相同网格及截面节点编号")

    mesh_path = next((trimuse_directory / "mesh").glob("*.vtk"))
    boundaries = BoundaryGeometry(mesh_path, dimension).section(center, normal)
    first, second = section_basis(normal)
    boundaries = [(name, [np.column_stack(((segment - center) @ first, (segment - center) @ second))
                          for segment in segments]) for name, segments in boundaries]

    def frame_values(frame):
        index, comsol_index, single_index, _, _ = frame
        section = SectionPlotter.sample_section(reader.read_frame("temperature", index), center, normal)
        calculated = vtk_to_numpy(section.GetPointData().GetArray("temperature"))[point_ids].copy()
        reference_values = interpolation @ comsol.values(comsol_index)
        if single_reader:
            single = SectionPlotter.sample_section(single_reader.read_frame("temperature", single_index), center, normal)
            single_values = vtk_to_numpy(single.GetPointData().GetArray("temperature"))[point_ids].copy()
            return reference_values, calculated, single_values
        return reference_values, calculated, None

    lower, upper, error_limit = np.inf, -np.inf, 0.0
    for frame in frames:
        reference_values, calculated, single_values = frame_values(frame)
        for values in (reference_values, calculated, single_values):
            if values is not None:
                lower, upper = min(lower, values.min()), max(upper, values.max())
        error_limit = max(error_limit, np.abs(reference_values - calculated).max())
        if single_values is not None:
            error_limit = max(error_limit, np.abs(reference_values - single_values).max())
    if upper == lower:
        lower, upper = lower - 0.5, upper + 0.5
    error_limit = error_limit or 1e-12
    print(f"固定温度色标 {lower:.6g} ~ {upper:.6g} K; 固定温差色标 ±{error_limit:.6g} K", flush=True)
    destination.mkdir(parents=True, exist_ok=True)
    paths = []
    historical_errors = np.zeros(2 if single_reader else 1)
    for count, frame in enumerate(frames, start=1):
        reference_values, calculated, single_values = frame_values(frame)
        differences = [reference_values - calculated] if single_values is None else [
            reference_values - single_values, reference_values - calculated]
        historical_errors = np.maximum(historical_errors, [np.abs(values).max() for values in differences])
        _, _, _, time, name = frame
        paths.append(plot_comparison(triangulation, reference_values, calculated, time,
                                     (lower, upper), error_limit, boundaries, destination / f"{name}.png",
                                     single_values=single_values, historical_errors=tuple(historical_errors),
                                     annotation=annotation))
        print(f"对比绘图 {count}/{len(frames)}, t={time:.6g} s; 历史最大绝对温差 {historical_errors} K", flush=True)
    return save_gif(paths, destination / "comparison.gif", duration=duration)


if __name__ == "__main__":
    case_directory = project_directory / "cases/transient_half_heat_2d"
    compare_temperature_fields(
        case_directory / "transient_half_heat_2d_comsol.vtu",
        case_directory / "output_transient_half_heat_2d/seed_202609134",
    )
