from __future__ import annotations
from pathlib import Path
from typing import TYPE_CHECKING
import numpy as np

from vtkmodules.vtkCommonCore import reference
from vtkmodules.vtkCommonDataModel import vtkGenericCell, vtkStaticCellLocator

from .output_reader import OutputReader
from .plot_style import plot_style, property_label, annotate_details, coordinate_labels

if TYPE_CHECKING:
    from vtkmodules.vtkCommonDataModel import vtkUnstructuredGrid

class PointPlotter:
    """点插值、时间序列提取、CSV 保存与绘图分别处理"""

    def __init__(self, output_directory: str | Path, postprocess_directory: str | Path, reader: OutputReader, center: tuple[float, float, float], property_metadata: dict[str, dict[str, str]] | None = None) -> None:
        self.output_directory = Path(output_directory).expanduser().resolve()
        self.postprocess_directory = Path(postprocess_directory).expanduser().resolve()
        self.reader = reader
        self.center = np.asarray(center, dtype=float)
        self.property_metadata = property_metadata if property_metadata is not None else {}

    def sample_points(self, mesh: vtkUnstructuredGrid, points: list[tuple[float, float, float]] | np.ndarray, property_name: str, locator: vtkStaticCellLocator) -> tuple[list[float], list[float]]:
        """返回对应各点的 (属性值列表, 到中心的距离列表); 网格及编号须一致, 坐标和距离单位 m"""
        cell = vtkGenericCell()
        weights = [0.0] * mesh.GetMaxCellSize()
        parametric_coordinates = [0.0, 0.0, 0.0]
        sub_id = reference(0)
        tolerance = 1e-9 * mesh.GetLength()

        point_values = mesh.GetPointData().GetArray(property_name)
        values = point_values if point_values is not None else mesh.GetCellData().GetArray(property_name)

        value_list = []
        radius_list = []
        for point in points:
            cell_id = locator.FindCell(point, tolerance ** 2, cell, sub_id, parametric_coordinates, weights)
            if cell_id < 0:
                raise ValueError(f"采样点 {point} 不在计算域内: {property_name}")
            if point_values is not None:
                value = sum(weights[index] * values.GetTuple1(cell.GetPointId(index)) for index in range(cell.GetNumberOfPoints()))
            else:
                # 单元数据按所在单元取值, 不擅自转换为节点平均值。
                value = values.GetTuple1(cell_id)
            value_list.append(value)
            radius_list.append(float(np.linalg.norm(np.asarray(point) - self.center)))
        return value_list, radius_list

    def extract(self, point: tuple[float, float, float], property_name: str, locator: vtkStaticCellLocator) -> list[tuple[float, float]]:
        """复用外部定位器逐帧采样, 返回 (时间, 属性值), 不写文件"""
        rows = []
        for time, mesh in self.reader.iter_frames(property_name):
            value_list, _ = self.sample_points(mesh, [point], property_name, locator)
            rows.append((time, value_list[0]))
        return rows

    def save(self, rows: list[tuple[float, float]], property_name: str, point_name: str) -> np.ndarray:
        """转换为两列数组并保存 CSV, 不在对象内绑定某个点的数据"""
        data = np.asarray(rows, dtype=float).reshape(-1, 2)
        directory = self.postprocess_directory / "points" / point_name
        directory.mkdir(parents=True, exist_ok=True)
        np.savetxt(directory / f"{property_name}.csv", data, delimiter=",", header=f"time_s,{property_name}", comments="")
        return data

    @plot_style
    def plot(self, data: np.ndarray, property_name: str, point_name: str, point: tuple[float, float, float] | None = None) -> Path:
        """只绘制传入数据, 不读取场或执行插值"""
        from matplotlib.figure import Figure
        from matplotlib.backends.backend_agg import FigureCanvasAgg

        figure = Figure()
        FigureCanvasAgg(figure)
        axis = figure.subplots()
        axis.plot(data[:, 0], data[:, 1])
        axis.set_xlabel("Time (s)", fontstyle="italic")
        axis.set_ylabel(property_label(property_name, self.property_metadata), fontstyle="italic")
        axis.grid(False)
        if point is not None:
            annotate_details(axis, coordinate_labels(point), position=(0.03, 0.94), align="left")
        figure.tight_layout()
        directory = self.postprocess_directory / "points" / point_name
        directory.mkdir(parents=True, exist_ok=True)
        path = directory / f"{property_name}.png"
        figure.savefig(path)
        return path
