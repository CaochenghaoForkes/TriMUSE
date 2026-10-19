from __future__ import annotations
from collections.abc import Iterator
from pathlib import Path
from typing import TYPE_CHECKING
import numpy as np

from vtkmodules.util.numpy_support import vtk_to_numpy
from vtkmodules.vtkCommonDataModel import vtkPlane
from vtkmodules.vtkFiltersCore import vtkCutter, vtkTriangleFilter
from vtkmodules.vtkFiltersGeometry import vtkDataSetSurfaceFilter

from .output_reader import OutputReader
from .plot_style import plot_style, annotate_time

if TYPE_CHECKING:
    from vtkmodules.vtkCommonDataModel import vtkPolyData, vtkUnstructuredGrid


class SectionPlotter:
    """二维取整个计算截面, 三维按平面切片; 提取、保存与云图分别处理"""

    def __init__(self, output_directory: str | Path, postprocess_directory: str | Path, reader: OutputReader, center: tuple[float, float, float]) -> None:
        self.output_directory = Path(output_directory).expanduser().resolve()
        self.postprocess_directory = Path(postprocess_directory).expanduser().resolve()
        self.reader = reader
        self.center = np.asarray(center, dtype=float)

    @staticmethod
    def sample_section(mesh: vtkUnstructuredGrid, center: tuple[float, float, float], normal: tuple[float, float, float]) -> vtkPolyData:
        """保留截面实际单元连接关系, 不用散点重新三角化跨越计算域边界"""
        if mesh.GetCell(0).GetCellDimension() == 2:
            section = vtkDataSetSurfaceFilter()
            section.SetNonlinearSubdivisionLevel(2)
        else:
            plane = vtkPlane()
            plane.SetOrigin(center)
            plane.SetNormal(normal)
            section = vtkCutter()
            section.SetCutFunction(plane)
        section.SetInputData(mesh)
        triangles = vtkTriangleFilter()
        triangles.SetInputConnection(section.GetOutputPort())
        triangles.PassLinesOff()
        triangles.PassVertsOff()
        triangles.Update()
        result = triangles.GetOutput()
        if result.GetNumberOfPolys() == 0:
            raise ValueError("指定平面未与计算域形成截面")
        return result

    def extract(self, center: tuple[float, float, float], normal: tuple[float, float, float], property_name: str) -> Iterator[tuple[str, float, vtkPolyData]]:
        """逐帧返回 (原始帧目录名, 时间, 带属性的三角形截面)"""
        for index, (time, mesh) in enumerate(self.reader.iter_frames(property_name)):
            frame_directory = self.reader.series[property_name][index][1].parent.name
            yield frame_directory, time, self.sample_section(mesh, center, normal)

    def save(self, section: vtkPolyData, property_name: str, section_name: str, frame_directory: str) -> Path:
        """保存编号、xyz、r、值; 单元属性使用三角形中心坐标, 不转换为节点平均值"""
        coordinates = vtk_to_numpy(section.GetPoints().GetData())
        values = section.GetPointData().GetArray(property_name)
        if values is None:
            triangles = vtk_to_numpy(section.GetPolys().GetData()).reshape(-1, 4)[:, 1:]
            coordinates = coordinates[triangles].mean(axis=1)
            values = section.GetCellData().GetArray(property_name)
        radii = np.linalg.norm(coordinates - self.center, axis=1)
        data = np.column_stack((np.arange(len(coordinates)), coordinates, radii, vtk_to_numpy(values)))
        directory = self.postprocess_directory / frame_directory / "sections" / section_name
        directory.mkdir(parents=True, exist_ok=True)
        path = directory / f"{property_name}.csv"
        np.savetxt(path, data, delimiter=",", fmt=["%d", "%.18e", "%.18e", "%.18e", "%.18e", "%.18e"],
                   header=f"index,x_m,y_m,z_m,r_m,{property_name}", comments="")
        return path

    @plot_style
    def plot(self, section: vtkPolyData, property_name: str, section_name: str, frame_directory: str, time: float, normal: tuple[float, float, float], regions: list | None = None) -> Path:
        """在截面内显示标量云图, 标注时间; 2D 使用 xy, 3D 投影至平面正交坐标"""
        from matplotlib.figure import Figure
        from matplotlib.backends.backend_agg import FigureCanvasAgg
        from matplotlib.tri import Triangulation
        from matplotlib.collections import LineCollection

        coordinates = vtk_to_numpy(section.GetPoints().GetData())
        normal = np.asarray(normal, dtype=float)
        normal = normal / np.linalg.norm(normal)
        axis = np.eye(3)[np.argmin(np.abs(normal))]
        first = axis - np.dot(axis, normal) * normal
        first /= np.linalg.norm(first)
        second = np.cross(normal, first)
        relative = coordinates - self.center
        triangles = vtk_to_numpy(section.GetPolys().GetData()).reshape(-1, 4)[:, 1:]
        triangulation = Triangulation(relative @ first, relative @ second, triangles)
        figure = Figure()
        FigureCanvasAgg(figure)
        axes = figure.subplots()
        values = section.GetPointData().GetArray(property_name)
        if values is not None:
            image = axes.tripcolor(triangulation, vtk_to_numpy(values), shading="gouraud")
        else:
            image = axes.tripcolor(triangulation, facecolors=vtk_to_numpy(section.GetCellData().GetArray(property_name)), shading="flat")
        figure.colorbar(image, ax=axes, label=property_name)
        if regions:
            labels = []
            for label, segments, position in regions:
                projected = [np.column_stack(((segment - self.center) @ first, (segment - self.center) @ second)) for segment in segments]
                axes.add_collection(LineCollection(projected, colors="black", linewidths=0.65))
                relative_position = position - self.center
                labels.append((label, (relative_position @ first, relative_position @ second)))
            # 薄涂层内放不下完整文字, 在右侧留白排放标签, 引线指向真实区域内。
            lower, upper = axes.get_xlim()
            axes.set_xlim(lower, upper + 1.1 * (upper - lower))
            for (label, position), height in zip(sorted(labels, key=lambda item: item[1][1]), np.linspace(0.20, 0.70, len(labels))):
                axes.annotate(label, xy=position, xytext=(0.97, height), textcoords="axes fraction",
                              fontsize=12, ha="right", va="center", annotation_clip=False,
                              bbox={"facecolor": "white", "edgecolor": "none", "pad": 1.0},
                              arrowprops={"arrowstyle": "-", "linewidth": 0.7, "color": "black"})
        axes.set_aspect("equal")
        axes.set_xlabel("u(m)")
        axes.set_ylabel("v(m)")
        annotate_time(axes, time)
        figure.tight_layout()
        directory = self.postprocess_directory / frame_directory / "sections" / section_name
        directory.mkdir(parents=True, exist_ok=True)
        path = directory / f"{property_name}.png"
        figure.savefig(path)
        return path
