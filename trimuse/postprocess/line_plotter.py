from __future__ import annotations
from collections.abc import Iterator
from pathlib import Path
from typing import TYPE_CHECKING
import numpy as np
from vtkmodules.vtkCommonCore import reference
from vtkmodules.vtkCommonDataModel import vtkGenericCell

from .output_reader import OutputReader
from .point_plotter import PointPlotter

if TYPE_CHECKING:
    from vtkmodules.vtkCommonDataModel import vtkStaticCellLocator, vtkUnstructuredGrid


class LinePlotter:
    """复用点插值, 分别处理线采样、逐帧提取、保存和绘图"""

    def __init__(self, output_directory: str | Path, postprocess_directory: str | Path, reader: OutputReader, center: tuple[float, float, float]) -> None:
        self.output_directory = Path(output_directory).expanduser().resolve()
        self.postprocess_directory = Path(postprocess_directory).expanduser().resolve()
        self.reader = reader
        self.point_plotter = PointPlotter(output_directory, postprocess_directory, reader, center)

    @staticmethod
    def find_endpoints(mesh: vtkUnstructuredGrid, center: tuple[float, float, float], direction: tuple[float, float, float], locator: vtkStaticCellLocator) -> tuple[tuple[float, float, float], tuple[float, float, float]]:
        """沿指定方向寻找域内连续线段的两端; 1D 直接取整个径向范围, 固定网格只需调用一次"""
        bounds = np.asarray(mesh.GetBounds()).reshape(3, 2)
        if mesh.GetCell(0).GetCellDimension() == 1:
            return tuple(bounds[:, 0]), tuple(bounds[:, 1])

        center = np.asarray(center, dtype=float)
        direction = np.asarray(direction, dtype=float)
        direction = direction / np.linalg.norm(direction)
        # 两端均延伸到包围盒外, 从相反方向分别寻找第一次进入计算域的位置。
        reach = np.linalg.norm(center - bounds.mean(axis=1)) + mesh.GetLength()
        start, end = center - reach * direction, center + reach * direction
        endpoints = []
        for first, last in ((start, end), (end, start)):
            position = [0.0, 0.0, 0.0]
            hit = locator.IntersectWithLine(first, last, 1e-9 * mesh.GetLength(), reference(0.0), position, [0.0, 0.0, 0.0], reference(0), reference(0), vtkGenericCell())
            if not hit:
                raise ValueError("指定直线未与计算域相交")
            endpoints.append(tuple(position))
        return endpoints[0], endpoints[1]

    @staticmethod
    def make_points(start: tuple[float, float, float], end: tuple[float, float, float], sample_count: int) -> np.ndarray:
        """在域内线段的两个端点间等距采样, 包含端点; 坐标单位 m"""
        return np.linspace(start, end, sample_count)

    def sample_line(self, mesh: vtkUnstructuredGrid, points: list[tuple[float, float, float]] | np.ndarray, property_name: str, locator: vtkStaticCellLocator) -> np.ndarray:
        """按输入点的顺序返回六列: 编号、x、y、z、r、属性值; 编号从 0 开始"""
        values, radii = self.point_plotter.sample_points(mesh, points, property_name, locator)
        coordinates = np.asarray(points, dtype=float).reshape(-1, 3)
        return np.column_stack((np.arange(len(coordinates)), coordinates, radii, values))

    def extract(self, center: tuple[float, float, float], direction: tuple[float, float, float], sample_count: int, property_name: str, locator: vtkStaticCellLocator) -> Iterator[tuple[str, float, np.ndarray]]:
        """首帧按线中心和方向布点, 后续复用; 逐帧返回 (原始帧目录名, 时间, 六列数据)"""
        mesh = self.reader.read_frame(property_name, 0)
        start, end = self.find_endpoints(mesh, center, direction, locator)
        points = self.make_points(start, end, sample_count)
        for index, (time, mesh) in enumerate(self.reader.iter_frames(property_name)):
            frame_directory = self.reader.series[property_name][index][1].parent.name
            yield frame_directory, time, self.sample_line(mesh, points, property_name, locator)

    def save(self, data: np.ndarray, property_name: str, line_name: str, frame_directory: str) -> Path:
        """保存到 原始帧目录名/lines/线名/属性.csv, 坐标及 r 的单位为 m"""
        directory = self.postprocess_directory / frame_directory / "lines" / line_name
        directory.mkdir(parents=True, exist_ok=True)
        path = directory / f"{property_name}.csv"
        np.savetxt(path, data, delimiter=",", fmt=["%d", "%.18e", "%.18e", "%.18e", "%.18e", "%.18e"], header=f"index,x_m,y_m,z_m,r_m,{property_name}", comments="")
        return path

    def plot(self, data: np.ndarray, property_name: str, line_name: str, frame_directory: str, time: float) -> Path:
        """以采样点编号为横轴, 在标题标注当前模拟时间, 不重新读取或采样"""
        from matplotlib.figure import Figure
        from matplotlib.backends.backend_agg import FigureCanvasAgg

        figure = Figure(figsize=(6.4, 4.2))
        FigureCanvasAgg(figure)
        axis = figure.subplots()
        axis.plot(data[:, 0], data[:, 5])
        axis.set_xlabel("Point index")
        axis.set_ylabel(property_name)
        axis.set_title(f"{line_name}, t = {time:.8g} s")
        axis.grid(True, alpha=0.3)
        figure.tight_layout()
        directory = self.postprocess_directory / frame_directory / "lines" / line_name
        directory.mkdir(parents=True, exist_ok=True)
        path = directory / f"{property_name}.png"
        figure.savefig(path, dpi=200)
        return path
