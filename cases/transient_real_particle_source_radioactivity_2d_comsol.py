"""稳态温度 + 瞬态浓度六联图；差值统一为 COMSOL - TriMUSE。

复用已有 VTU 单元插值、截面、边界和 GIF 实现，不重新散点三角化。
COMSOL 浓度单位必须明确指定；无时间标签的单帧必须显式给出时间。
"""

import argparse
from pathlib import Path
import re
import sys

import numpy as np
from vtkmodules.util.numpy_support import vtk_to_numpy
from vtkmodules.vtkIOXML import vtkXMLUnstructuredGridReader

sys.path.insert(0, str(Path(__file__).resolve().parent))
from transient_half_heat_2d_comsol import (
    OutputReader, SectionPlotter, BoundaryGeometry, prepare_comparison_grid,
    section_basis, save_gif, plot_style, annotate_time, scientific_axes, BOUNDARY_COLORS,
)

AVOGADRO = 6.02214076e23


class RadioactivityComsolReader:
    """读取静态温度和浓度序列；两者可分别来自不同的 VTU 网格。"""

    def __init__(self, path: str | Path, concentration_unit: str,
                 single_time: float | None = None,
                 temperature_array: str = "温度", concentration_array: str = "浓度",
                 temperature_path: str | Path | None = None) -> None:
        self.mesh = self.read_mesh(path)
        self.temperature_mesh = self.read_mesh(temperature_path) if temperature_path else self.mesh
        self.factor = {"atom": 1.0, "mol": AVOGADRO}[concentration_unit]

        def arrays(mesh, label):
            data = mesh.GetPointData()
            matches = []
            for index in range(data.GetNumberOfArrays()):
                name = data.GetArrayName(index)
                normalized = name.replace("_", " ")
                time = re.search(r"@\s*t\s*=\s*([+\-\d.eE]+)", normalized)
                if (name == label or normalized.split("@")[0].strip().startswith(label)) and data.GetArray(index).GetNumberOfComponents() == 1:
                    matches.append((float(time[1]) if time else None, name))
            if not matches:
                raise ValueError(f"没有找到数组 {label!r}；可用数组: {[data.GetArrayName(i) for i in range(data.GetNumberOfArrays())]}")
            return matches

        temperatures = arrays(self.temperature_mesh, temperature_array)
        # 温度为稳态，使用导出的静态数组或最后一个温度数组。
        self.temperature_name = max(temperatures, key=lambda item: -np.inf if item[0] is None else item[0])[1]
        concentrations = arrays(self.mesh, concentration_array)
        if any(time is None for time, _ in concentrations):
            if len(concentrations) != 1 or single_time is None:
                raise ValueError("COMSOL 浓度无时间标签，不能推断对应时刻。请重新导出全部时间，或用 --comsol-time 显式指定单帧时间。")
            concentrations = [(single_time, concentrations[0][1])]
        self.concentrations = sorted(concentrations)
        self.times = np.array([time for time, _ in self.concentrations])
        if len(np.unique(self.times)) != len(self.times):
            raise ValueError("存在重复浓度时刻，请通过 --concentration-array 明确指定导出的浓度数组。")
        print(f"COMSOL: {self.mesh.GetNumberOfPoints()} 点，{len(self.times)} 个浓度时刻；浓度单位 {concentration_unit}/m³", flush=True)

    @staticmethod
    def read_mesh(path):
        reader = vtkXMLUnstructuredGridReader()
        reader.SetFileName(str(Path(path).expanduser().resolve()))
        errors = []
        reader.AddObserver("ErrorEvent", lambda *_: errors.append(True))
        reader.Update()
        if errors or reader.GetErrorCode():
            raise ValueError(f"COMSOL VTU 读取失败: {path}")
        return reader.GetOutput()

    def temperature(self) -> np.ndarray:
        return vtk_to_numpy(self.temperature_mesh.GetPointData().GetArray(self.temperature_name))

    def concentration(self, index: int) -> np.ndarray:
        return vtk_to_numpy(self.mesh.GetPointData().GetArray(self.concentrations[index][1])) * self.factor


@plot_style
def plot_fields(rows, time: float, limits, error_limits, historical_errors,
                boundaries, path: Path) -> Path:
    """每排依次为 TriMUSE、COMSOL、COMSOL - TriMUSE；色标跨帧固定。"""
    from matplotlib.figure import Figure
    from matplotlib.backends.backend_agg import FigureCanvasAgg
    from matplotlib.colors import Normalize
    from matplotlib.collections import LineCollection
    from matplotlib.lines import Line2D
    from mpl_toolkits.axes_grid1 import make_axes_locatable

    figure = Figure(figsize=(16, 12))
    FigureCanvasAgg(figure)
    axes = figure.subplots(2, 3)
    for row, ((grid, calculated, reference_values), field, unit, cmap) in enumerate(zip(
            rows, ("Temperature", "Concentration"), ("K", "atom/m³"), ("jet", "viridis"))):
        for column, (values, title) in enumerate(zip(
                (calculated, reference_values, reference_values - calculated),
                ("TriMUSE", "COMSOL", "COMSOL - TriMUSE"))):
            axis = axes[row, column]
            difference = column == 2
            norm = Normalize(-error_limits[row], error_limits[row]) if difference else Normalize(*limits[row])
            image = axis.tripcolor(grid, values, shading="gouraud", cmap="PuOr_r" if difference else cmap, norm=norm)
            color_axis = make_axes_locatable(axis).append_axes("right", size="5%", pad=0.10)
            colorbar = figure.colorbar(image, cax=color_axis)
            colorbar.set_label(f"{'Δ' if difference else ''}{field} ({unit})", fontstyle="italic", fontsize=12)
            colorbar.ax.tick_params(labelsize=9)
            handles = []
            for index, (name, segments) in enumerate(boundaries):
                color = BOUNDARY_COLORS[index % len(BOUNDARY_COLORS)]
                axis.add_collection(LineCollection(segments, colors=color, linewidths=0.7))
                handles.append(Line2D([], [], color=color, label=name, linewidth=1))
            dx, dy = np.ptp(grid.x), np.ptp(grid.y)
            axis.set_xlim(grid.x.min() - .05 * dx, grid.x.max() + .40 * dx)
            axis.set_ylim(grid.y.min() - .12 * dy, grid.y.max() + .04 * dy)
            axis.set_aspect("equal")
            axis.set_title(f"{field}: {title}", fontsize=12)
            axis.set_xlabel("u (m)", fontstyle="italic")
            if column == 0:
                axis.set_ylabel("v (m)", fontstyle="italic")
            if handles:
                axis.legend(handles=handles, loc="lower left", ncols=2, fontsize=6, frameon=False)
            details = ["steady"] if row == 0 else []
            if difference:
                details.append(f"Historical max |Δ|\n{historical_errors[row]:.3e} {unit}")
            # 稳态温度不标成随浓度时间推进的温度解。
            if row == 0:
                axis.text(.96, .96, "\n".join(details), transform=axis.transAxes,
                          ha="right", va="top", fontsize=9, fontstyle="italic")
            else:
                annotate_time(axis, time, details=details, fontsize=9)
            scientific_axes(axis)
            axis.tick_params(labelsize=10)
    figure.tight_layout()
    figure.savefig(path, dpi=130)
    return path


def compare_fields(vtu_path: str | Path, trimuse_directory: str | Path, *,
                   concentration_unit: str, output_directory: str | Path | None = None,
                   comsol_time: float | None = None, times: list[float] | None = None,
                   temperature_array: str = "温度", concentration_array: str = "浓度",
                   temperature_vtu_path: str | Path | None = None, duration: int = 120) -> Path:
    """仅匹配真实共同时间，不做时间插值、不把单帧复制为时间序列。"""
    root = Path(trimuse_directory).expanduser().resolve()
    destination = Path(output_directory).expanduser().resolve() if output_directory else root / "postprocess/comsol_comparison"
    comsol = RadioactivityComsolReader(vtu_path, concentration_unit, comsol_time, temperature_array, concentration_array, temperature_vtu_path)
    reader = OutputReader(root, ["temperature", "concentration"])
    frames = []
    for index, (time, frame_path) in enumerate(reader.series["concentration"]):
        matches = np.flatnonzero(np.isclose(comsol.times, time, rtol=0, atol=1e-7))
        if len(matches) and (times is None or any(np.isclose(time, value, rtol=0, atol=1e-7) for value in times)):
            frames.append((index, int(matches[0]), time, frame_path.parent.name))
    if not frames:
        raise ValueError("没有共同输出时刻；请核对 COMSOL 时间单位和 TriMUSE 的 PVD。")

    center, normal = (0., 0., 0.), (0., 0., 1.)
    temperature_mesh = reader.read_frame("temperature", len(reader.series["temperature"]) - 1)
    temperature_section = SectionPlotter.sample_section(temperature_mesh, center, normal)
    temperature_grid, temperature_ids, temperature_weights = prepare_comparison_grid(temperature_section, comsol.temperature_mesh, center, normal)
    temperature = vtk_to_numpy(temperature_section.GetPointData().GetArray("temperature"))[temperature_ids].copy()
    reference_temperature = temperature_weights @ comsol.temperature()
    temperature_row = (temperature_grid, temperature, reference_temperature)

    def concentration_section(index):
        return SectionPlotter.sample_section(reader.read_frame("concentration", index), center, normal)

    section = concentration_section(frames[0][0])
    grid, point_ids, weights = prepare_comparison_grid(section, comsol.mesh, center, normal)
    first, second = section_basis(normal)
    boundaries = BoundaryGeometry(next((root / "mesh").glob("*.vtk")), 2).section(center, normal)
    boundaries = [(name, [np.column_stack((segment @ first, segment @ second)) for segment in segments]) for name, segments in boundaries]

    def concentration_values(frame):
        index, comsol_index, _, _ = frame
        section = concentration_section(index)
        calculated = vtk_to_numpy(section.GetPointData().GetArray("concentration"))[point_ids].copy()
        return calculated, weights @ comsol.concentration(comsol_index)

    lower, upper, error = np.inf, -np.inf, 0.
    for frame in frames:
        calculated, reference_values = concentration_values(frame)
        lower = min(lower, calculated.min(), reference_values.min())
        upper = max(upper, calculated.max(), reference_values.max())
        error = max(error, np.max(np.abs(reference_values - calculated)))
    limits = [(min(temperature.min(), reference_temperature.min()), max(temperature.max(), reference_temperature.max())), (lower, upper)]
    limits = [(low, high) if high > low else (low - .5, high + .5) for low, high in limits]
    temperature_error = float(np.max(np.abs(reference_temperature - temperature)))
    error_limits = (temperature_error or 1e-12, error or 1e-12)
    destination.mkdir(parents=True, exist_ok=True)
    historical = [temperature_error, 0.]
    paths = []
    for count, frame in enumerate(frames, 1):
        calculated, reference_values = concentration_values(frame)
        historical[1] = max(historical[1], np.max(np.abs(reference_values - calculated)))
        _, _, time, name = frame
        paths.append(plot_fields((temperature_row, (grid, calculated, reference_values)), time,
                                 limits, error_limits, historical, boundaries, destination / f"{name}.png"))
        print(f"六联图 {count}/{len(frames)}，t={time:g} s；最大绝对差: T={temperature_error:.6g} K, c={historical[1]:.6g} atom/m³", flush=True)
    if len(paths) == 1:
        print("只有一个共同时间，已保存单帧 PNG，不生成伪时间动画。", flush=True)
        return paths[0]
    return save_gif(paths, destination / "comparison.gif", duration=duration)


if __name__ == "__main__":
    case = Path(__file__).resolve().parent / "transient_real_particle_source_radioactivity_2d"
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--comsol", type=Path, default=case / "radioactivity_c.vtu", help="浓度时间序列 VTU")
    parser.add_argument("--comsol-temperature", type=Path, default=case / "radioactivity_T.vtu", help="稳态温度 VTU")
    parser.add_argument("--trimuse", type=Path, default=case / "output_transient_real_particle_source_radioactivity_2d/seed_202609134")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--concentration-unit", choices=("atom", "mol"), required=True, help="VTU 中浓度的实际单位：atom/m³ 或 mol/m³")
    parser.add_argument("--comsol-time", type=float, help="仅用于无时间标签的单帧；单位 s，不自动假定为初始或末时刻")
    parser.add_argument("--times", type=float, nargs="+")
    parser.add_argument("--temperature-array", default="温度")
    parser.add_argument("--concentration-array", default="浓度")
    options = parser.parse_args()
    print(compare_fields(options.comsol, options.trimuse, concentration_unit=options.concentration_unit,
                         output_directory=options.output, comsol_time=options.comsol_time,
                         times=options.times, temperature_array=options.temperature_array,
                         concentration_array=options.concentration_array,
                         temperature_vtu_path=options.comsol_temperature))
