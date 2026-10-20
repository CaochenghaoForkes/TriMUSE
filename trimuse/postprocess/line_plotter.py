from __future__ import annotations
from collections.abc import Iterator
from pathlib import Path
from typing import TYPE_CHECKING
import numpy as np
from vtkmodules.vtkCommonCore import reference
from vtkmodules.vtkCommonDataModel import vtkGenericCell

from .output_reader import OutputReader
from .point_plotter import PointPlotter
from .plot_style import plot_style, annotate_time, property_label, BOUNDARY_COLORS, scientific_axes

if TYPE_CHECKING:
    from vtkmodules.vtkCommonDataModel import vtkStaticCellLocator, vtkUnstructuredGrid


class LinePlotter:
    """复用点插值, 分别处理线采样、逐帧提取、保存和绘图"""

    def __init__(self, output_directory: str | Path, postprocess_directory: str | Path, reader: OutputReader, center: tuple[float, float, float], property_metadata: dict[str, dict[str, str]] | None = None) -> None:
        self.output_directory = Path(output_directory).expanduser().resolve()
        self.postprocess_directory = Path(postprocess_directory).expanduser().resolve()
        self.reader = reader
        self.point_plotter = PointPlotter(output_directory, postprocess_directory, reader, center, property_metadata)
        self.property_metadata = self.point_plotter.property_metadata
        self.boundary_colors: dict[str, str] = {}

    @staticmethod
    def find_endpoints(mesh: vtkUnstructuredGrid, center: tuple[float, float, float], direction: tuple[float, float, float], locator: vtkStaticCellLocator) -> tuple[tuple[float, float, float], tuple[float, float, float]]:
        """起点固定为颗粒中心, 沿指定方向找到外表面终点; 固定网格只需调用一次"""
        bounds = np.asarray(mesh.GetBounds()).reshape(3, 2)
        if mesh.GetCell(0).GetCellDimension() == 1:
            return tuple(center), tuple(bounds[:, 1])

        center = np.asarray(center, dtype=float)
        direction = np.asarray(direction, dtype=float)
        direction = direction / np.linalg.norm(direction)
        # 从射线远端朝中心求交, 取得该方向的外表面位置。
        reach = np.linalg.norm(center - bounds.mean(axis=1)) + mesh.GetLength()
        outside = center + reach * direction
        tolerance = 1e-9 * mesh.GetLength()
        position = [0.0, 0.0, 0.0]
        hit = locator.IntersectWithLine(outside, center, tolerance, reference(0.0), position, [0.0, 0.0, 0.0], reference(0), reference(0), vtkGenericCell())
        if not hit:
            raise ValueError("指定方向未与计算域外表面相交")
        cell_id = locator.FindCell(position, tolerance ** 2, vtkGenericCell(), reference(0),
                                   [0.0, 0.0, 0.0], [0.0] * mesh.GetMaxCellSize())
        if cell_id < 0:
            position = np.asarray(position) - tolerance * direction
        return tuple(center), tuple(position)

    @staticmethod
    def make_points(start: tuple[float, float, float], end: tuple[float, float, float], sample_count: int) -> np.ndarray:
        """在域内线段的两个端点间等距采样, 包含端点; 坐标单位 m"""
        return np.linspace(start, end, sample_count)

    def sample_line(self, mesh: vtkUnstructuredGrid, points: list[tuple[float, float, float]] | np.ndarray, property_name: str, locator: vtkStaticCellLocator) -> np.ndarray:
        """按输入点的顺序返回六列: 编号、x、y、z、r、属性值; 编号从 0 开始"""
        values, radii = self.point_plotter.sample_points(mesh, points, property_name, locator)
        coordinates = np.asarray(points, dtype=float).reshape(-1, 3)
        return np.column_stack((np.arange(len(coordinates)), coordinates, radii, values))

    def extract(self, direction: tuple[float, float, float], sample_count: int, property_name: str, locator: vtkStaticCellLocator) -> Iterator[tuple[str, float, np.ndarray]]:
        """从初始化指定的颗粒中心沿方向布点; 逐帧返回 (原始帧目录名, 时间, 六列数据)"""
        mesh = self.reader.read_frame(property_name, 0)
        start, end = self.find_endpoints(mesh, self.point_plotter.center, direction, locator)
        points = self.make_points(start, end, sample_count)
        for index, (time, mesh) in enumerate(self.reader.iter_frames(property_name)):
            frame_directory = self.reader.series[property_name][index][1].parent.name
            yield frame_directory, time, self.sample_line(mesh, points, property_name, locator)

    def save(self, data: np.ndarray, property_name: str, line_name: str, frame_directory: str) -> Path:
        """保存到 lines/线名/属性名/原始帧目录名.csv, 坐标及 r 的单位为 m"""
        directory = self.postprocess_directory / "lines" / line_name / property_name
        directory.mkdir(parents=True, exist_ok=True)
        path = directory / f"{frame_directory}.csv"
        np.savetxt(path, data, delimiter=",", fmt=["%d", "%.18e", "%.18e", "%.18e", "%.18e", "%.18e"], header=f"index,x_m,y_m,z_m,r_m,{property_name}", comments="")
        return path

    @plot_style
    def plot(self, data: np.ndarray, property_name: str, line_name: str, frame_directory: str, time: float, boundaries: list[tuple[str, float]] | None = None, value_limits: tuple[float, float] | None = None, annotation: list[str] | None = None) -> Path:
        """以到颗粒中心的距离 r 为横轴, 图内标注模拟时间"""
        from matplotlib.figure import Figure
        from matplotlib.backends.backend_agg import FigureCanvasAgg

        figure = Figure()
        FigureCanvasAgg(figure)
        axis = figure.subplots()
        axis.plot(data[:, 4], data[:, 5])
        axis.set_xlabel("r(m)", fontstyle="italic")
        axis.set_ylabel(property_label(property_name, self.property_metadata), fontstyle="italic")
        if value_limits is not None:
            lower, upper = value_limits
            span = upper - lower if upper > lower else max(abs(lower) * 0.02, 1e-12)
            axis.set_ylim(lower - 0.35 * span, upper + 0.08 * span)
        if boundaries:
            for name, radius in boundaries:
                color = self.boundary_colors.setdefault(name, BOUNDARY_COLORS[len(self.boundary_colors) % len(BOUNDARY_COLORS)])
                axis.axvline(radius, color=color, linewidth=1.0, linestyle="--", label=name)
            axis.legend(loc="lower left", ncols=2, fontsize=9, frameon=False, labelspacing=0.7)
        annotate_time(axis, time, details=annotation)
        axis.grid(False)
        scientific_axes(axis)
        figure.tight_layout()
        directory = self.postprocess_directory / "lines" / line_name / property_name
        directory.mkdir(parents=True, exist_ok=True)
        path = directory / f"{frame_directory}.png"
        figure.savefig(path)
        return path
