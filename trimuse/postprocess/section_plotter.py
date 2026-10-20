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
from .plot_style import plot_style, annotate_time, property_label, BOUNDARY_COLORS, scientific_axes

if TYPE_CHECKING:
    from vtkmodules.vtkCommonDataModel import vtkPolyData, vtkUnstructuredGrid


class SectionPlotter:
    """二维取整个计算截面, 三维按平面切片; 提取、保存与云图分别处理"""

    def __init__(self, output_directory: str | Path, postprocess_directory: str | Path, reader: OutputReader, center: tuple[float, float, float], property_metadata: dict[str, dict[str, str]] | None = None) -> None:
        self.output_directory = Path(output_directory).expanduser().resolve()
        self.postprocess_directory = Path(postprocess_directory).expanduser().resolve()
        self.reader = reader
        self.center = np.asarray(center, dtype=float)
        self.property_metadata = property_metadata if property_metadata is not None else {}
        self.boundary_colors: dict[str, str] = {}
        self.annotation_fontsize = 12

    @staticmethod
    def sample_section(mesh: vtkUnstructuredGrid, center: tuple[float, float, float], normal: tuple[float, float, float]) -> vtkPolyData:
        """保留截面实际单元连接关系, 不用散点重新三角化跨越计算域边界"""
        if mesh.GetCell(0).GetCellDimension() == 2: # 取第一个单元如果是 2d 的直接取原来的二维网格，无需再切割
            section = vtkDataSetSurfaceFilter()
            section.SetNonlinearSubdivisionLevel(2) # 对于二次等高阶单元，将曲边进一步细分成小面片，使显示的边界更平滑。这是后处理细分，不会改变有限元计算网格或重新求解
        else:
            plane = vtkPlane() # 定义一个经过 center、法向为 normal 的平面
            plane.SetOrigin(center)
            plane.SetNormal(normal)
            section = vtkCutter() # 创建切割器
            section.SetCutFunction(plane) # 指定用这个平面切割网格
        section.SetInputData(mesh) # 将原始网格交给上面创建的提取器或切割器
        triangles = vtkTriangleFilter() # 创建三角化过滤器
        # 切割四面体时，截面可能是三角形，也可能是四边形；这里统一拆成三角形，方便后面绘图
        triangles.SetInputConnection(section.GetOutputPort()) # 原始网格 → 提取二维面 / 切割三维网格 → 三角形面片
        # 不保留独立的线单元和点单元，只保留面片
        triangles.PassLinesOff()
        triangles.PassVertsOff()
        # 前面是配置 下面是真正执行整条处理流程
        triangles.Update()
        # 取得结果
        result = triangles.GetOutput()
        if result.GetNumberOfPolys() == 0: # 如果没有得到任何面片，就报错
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
        directory = self.postprocess_directory / "sections" / section_name / property_name
        directory.mkdir(parents=True, exist_ok=True)
        path = directory / f"{frame_directory}.csv"
        np.savetxt(path, data, delimiter=",", fmt=["%d", "%.18e", "%.18e", "%.18e", "%.18e", "%.18e"], header=f"index,x_m,y_m,z_m,r_m,{property_name}", comments="")
        return path

    @staticmethod
    def value_range(section: vtkPolyData, property_name: str) -> tuple[float, float]:
        values = section.GetPointData().GetArray(property_name) # 取得当前截面上指定属性的数值范围
        if values is None:
            values = section.GetCellData().GetArray(property_name)
        return values.GetRange()

    @plot_style
    def plot(self, section: vtkPolyData, property_name: str, section_name: str, frame_directory: str, time: float, normal: tuple[float, float, float], boundaries: list | None = None, value_limits: tuple[float, float] | None = None, annotation: list[str] | None = None) -> Path:
        """在截面内显示标量云图, 标注时间; 2D 使用 xy, 3D 投影至平面正交坐标"""
        from matplotlib.figure import Figure
        from matplotlib.backends.backend_agg import FigureCanvasAgg
        from matplotlib.tri import Triangulation
        from matplotlib.collections import LineCollection
        from matplotlib.lines import Line2D
        from matplotlib.colors import Normalize
        from mpl_toolkits.axes_grid1 import make_axes_locatable

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
        figure = Figure(figsize=(6.0, 6.5))
        FigureCanvasAgg(figure)
        axes = figure.subplots()
        norm = Normalize(*value_limits) if value_limits is not None else None
        color = self.property_metadata.get(property_name, {}).get("color", "viridis")
        values = section.GetPointData().GetArray(property_name)
        if values is not None:
            image = axes.tripcolor(triangulation, vtk_to_numpy(values), shading="gouraud", norm=norm, cmap=color)
        else:
            image = axes.tripcolor(triangulation, facecolors=vtk_to_numpy(section.GetCellData().GetArray(property_name)), shading="flat", norm=norm, cmap=color)
        # 随等比例主图一起布局, 避免色标保留原始子图高度而明显长于截面。
        divider = make_axes_locatable(axes)
        colorbar_axes = divider.append_axes("right", size="5%", pad=0.12)
        colorbar = figure.colorbar(image, cax=colorbar_axes)
        colorbar.set_label(property_label(property_name, self.property_metadata), fontstyle="italic")
        if boundaries:
            handles = []
            for label, segments in boundaries:
                color = self.boundary_colors.setdefault(label, BOUNDARY_COLORS[len(self.boundary_colors) % len(BOUNDARY_COLORS)])
                projected = [np.column_stack(((segment - self.center) @ first, (segment - self.center) @ second)) for segment in segments]
                axes.add_collection(LineCollection(projected, colors=color, linewidths=1.0))
                handles.append(Line2D([], [], color=color, linewidth=1.5, label=label))
            lower, upper = (relative @ second).min(), (relative @ second).max()
            axes.set_ylim(lower - 0.10 * (upper - lower), upper + 0.04 * (upper - lower))
            left, right = axes.get_xlim()
            axes.set_xlim(left, right + 0.35 * (right - left))
            axes.legend(handles=handles, loc="lower left", ncols=3, frameon=False,
                        fontsize=7, labelspacing=0.4, handlelength=1.3, columnspacing=0.6)
        axes.set_aspect("equal")
        axes.set_xlabel("u(m)", fontstyle="italic")
        axes.set_ylabel("v(m)", fontstyle="italic")
        annotate_time(axes, time, position=(0.90, 0.94), details=annotation, fontsize=self.annotation_fontsize)
        scientific_axes(axes)
        figure.tight_layout()
        directory = self.postprocess_directory / "sections" / section_name / property_name
        directory.mkdir(parents=True, exist_ok=True)
        path = directory / f"{frame_directory}.png"
        figure.savefig(path, bbox_inches="tight")
        return path
