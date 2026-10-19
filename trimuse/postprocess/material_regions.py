from pathlib import Path
import shlex
import numpy as np

from vtkmodules.util.numpy_support import vtk_to_numpy
from vtkmodules.vtkCommonCore import vtkIdList
from vtkmodules.vtkFiltersCore import vtkFeatureEdges, vtkExtractCells
from vtkmodules.vtkIOLegacy import vtkUnstructuredGridReader

from .section_plotter import SectionPlotter


class MaterialRegions:
    """读取原始网格的区域划分, 缓存截面上的材料界线和区域内标注位置"""

    def __init__(self, mesh_path: str | Path, dimension: int, materials: dict[str, str]) -> None:
        mesh_path = Path(mesh_path)
        reader = vtkUnstructuredGridReader()
        reader.SetFileName(str(mesh_path))
        reader.ReadAllScalarsOn()
        reader.Update()
        mesh = reader.GetOutput()
        attributes = vtk_to_numpy(mesh.GetCellData().GetArray("CellEntityIds"))
        # VTK 只含编号; 同目录 MSH 的 PhysicalNames 保存区域名。
        names = {}
        with mesh_path.with_suffix(".msh").open() as stream:
            for line in stream:
                if line.strip() == "$PhysicalNames":
                    for _ in range(int(next(stream))):
                        dim, tag, name = shlex.split(next(stream))
                        if int(dim) == dimension:
                            names[int(tag)] = name
                    break
        self.regions = []
        for attribute, name in names.items():
            ids = vtkIdList()
            for index in np.flatnonzero(attributes == attribute):
                if mesh.GetCell(int(index)).GetCellDimension() == dimension:
                    ids.InsertNextId(int(index))
            extract = vtkExtractCells()
            extract.SetInputData(mesh)
            extract.SetCellList(ids)
            extract.Update()
            label = name if materials[name] == name else f"{name} ({materials[name]})"
            self.regions.append((label, extract.GetOutput()))
        self.sections = {}

    def section(self, center: tuple[float, float, float], normal: tuple[float, float, float]) -> list:
        key = (tuple(center), tuple(normal))
        if key not in self.sections:
            regions = []
            unit_normal = np.asarray(normal, dtype=float) / np.linalg.norm(normal)
            axis = np.eye(3)[np.argmin(np.abs(unit_normal))]
            first = axis - np.dot(axis, unit_normal) * unit_normal
            first /= np.linalg.norm(first)
            upward = np.cross(unit_normal, first)
            for index, (label, mesh) in enumerate(self.regions):
                # 三维平面可能只穿过部分材料区域。
                try:
                    surface = SectionPlotter.sample_section(mesh, center, normal)
                except ValueError:
                    continue
                edges = vtkFeatureEdges()
                edges.SetInputData(surface)
                edges.BoundaryEdgesOn()
                edges.FeatureEdgesOff()
                edges.NonManifoldEdgesOff()
                edges.ManifoldEdgesOff()
                edges.Update()
                boundary = edges.GetOutput()
                segments = [np.array([boundary.GetPoint(boundary.GetCell(i).GetPointId(j))
                                      for j in range(boundary.GetCell(i).GetNumberOfPoints())])
                            for i in range(boundary.GetNumberOfCells())]
                coordinates = vtk_to_numpy(surface.GetPoints().GetData())
                triangles = vtk_to_numpy(surface.GetPolys().GetData()).reshape(-1, 4)[:, 1:]
                centers = coordinates[triangles].mean(axis=1)
                # 使用实际区域内的三角形中心, 不用可能落到另一层的环带质心。
                target = coordinates.mean(axis=0)
                target += upward * ((index + 0.5) / len(self.regions) - 0.5) * 0.6 * mesh.GetLength()
                position = centers[np.argmin(np.linalg.norm(centers - target, axis=1))]
                regions.append((label, segments, position))
            self.sections[key] = regions
        return self.sections[key]
