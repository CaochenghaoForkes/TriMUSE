from pathlib import Path
import shlex
import numpy as np

from vtkmodules.util.numpy_support import vtk_to_numpy
from vtkmodules.vtkCommonCore import vtkIdList, reference
from vtkmodules.vtkCommonDataModel import vtkPlane, vtkStaticCellLocator, vtkGenericCell
from vtkmodules.vtkFiltersCore import vtkCutter, vtkExtractCells
from vtkmodules.vtkFiltersGeometry import vtkDataSetSurfaceFilter
from vtkmodules.vtkIOLegacy import vtkUnstructuredGridReader


class BoundaryGeometry:
    """从原始网格读取命名界面和边界, 缓存它们在各个截面上的曲线"""

    def __init__(self, mesh_path: str | Path, dimension: int) -> None:
        mesh_path = Path(mesh_path)
        reader = vtkUnstructuredGridReader()
        reader.SetFileName(str(mesh_path))
        reader.ReadAllScalarsOn()
        reader.Update()
        mesh = reader.GetOutput()
        attributes = vtk_to_numpy(mesh.GetCellData().GetArray("CellEntityIds"))
        names = {}
        with mesh_path.with_suffix(".msh").open() as stream:
            for line in stream:
                if line.strip() == "$PhysicalNames":
                    for _ in range(int(next(stream))):
                        dim, tag, name = shlex.split(next(stream))
                        if int(dim) == dimension - 1:
                            names[int(tag)] = name
                    break
        self.boundaries = []
        for attribute, name in names.items():
            ids = vtkIdList()
            for index in np.flatnonzero(attributes == attribute):
                if mesh.GetCell(int(index)).GetCellDimension() == dimension - 1:
                    ids.InsertNextId(int(index))
            extract = vtkExtractCells()
            extract.SetInputData(mesh)
            extract.SetCellList(ids)
            extract.Update()
            self.boundaries.append((name, extract.GetOutput()))
        self.dimension = dimension
        self.length = mesh.GetLength()
        self.locators = {}
        if dimension > 1:
            for name, boundary in self.boundaries:
                locator = vtkStaticCellLocator()
                locator.SetDataSet(boundary)
                locator.BuildLocator()
                self.locators[name] = locator
        self.sections = {}
        self.rays = {}

    def radial_intersections(self, center: tuple[float, float, float], direction: tuple[float, float, float]) -> list[tuple[str, float]]:
        """返回从颗粒中心沿射线经过的命名界面位置; 共线的对称轴不作为径向截面"""
        key = (tuple(center), tuple(direction))
        if key not in self.rays:
            center = np.asarray(center, dtype=float)
            direction = np.asarray(direction, dtype=float)
            direction /= np.linalg.norm(direction)
            intersections = []
            for name, boundary in self.boundaries:
                if name == "symmetry_axis":
                    continue
                if self.dimension == 1:
                    position = np.asarray(boundary.GetPoint(0))
                else:
                    bounds = np.asarray(boundary.GetBounds()).reshape(3, 2)
                    reach = np.linalg.norm(center - bounds.mean(axis=1)) + self.length
                    outside = center + reach * direction
                    position = [0.0, 0.0, 0.0]
                    hit = self.locators[name].IntersectWithLine(outside, center, 1e-9 * self.length, reference(0.0),
                                                               position, [0.0, 0.0, 0.0], reference(0), reference(0), vtkGenericCell())
                    if not hit:
                        continue
                radius = float(np.linalg.norm(position - center))
                if radius > 1e-9 * self.length:
                    intersections.append((name, radius))
            self.rays[key] = sorted(intersections, key=lambda item: item[1])
        return self.rays[key]

    def section(self, center: tuple[float, float, float], normal: tuple[float, float, float]) -> list:
        key = (tuple(center), tuple(normal))
        if key not in self.sections:
            boundaries = []
            for name, mesh in self.boundaries:
                if self.dimension == 2:
                    curve = vtkDataSetSurfaceFilter()
                    curve.SetNonlinearSubdivisionLevel(3)
                else:
                    plane = vtkPlane()
                    plane.SetOrigin(center)
                    plane.SetNormal(normal)
                    curve = vtkCutter()
                    curve.SetCutFunction(plane)
                curve.SetInputData(mesh)
                curve.Update()
                result = curve.GetOutput()
                segments = [np.array([result.GetPoint(result.GetCell(i).GetPointId(j))
                                      for j in range(result.GetCell(i).GetNumberOfPoints())])
                            for i in range(result.GetNumberOfCells()) if result.GetCell(i).GetCellDimension() == 1]
                if segments:
                    boundaries.append((name, segments))
            self.sections[key] = boundaries
        return self.sections[key]
