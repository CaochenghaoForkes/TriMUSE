"""二维标量场 VTU 对比：TriMUSE、COMSOL、TriMUSE−COMSOL

--field 参数依次为 TriMUSE 字段名、COMSOL 字段名、图例标签、单位
两份数据应采用相同的坐标单位、物理量单位和物理状态
"""
from pathlib import Path
import argparse
from collections import Counter

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.tri as mtri
from matplotlib.collections import LineCollection
from matplotlib.colors import Normalize
from matplotlib.ticker import MaxNLocator
import numpy as np
import vtk
from vtk.util.numpy_support import vtk_to_numpy


def read_surface(path):
    reader = vtk.vtkXMLUnstructuredGridReader()
    reader.SetFileName(str(path))
    reader.Update()
    surface = vtk.vtkDataSetSurfaceFilter()
    surface.SetInputData(reader.GetOutput())
    surface.SetNonlinearSubdivisionLevel(1)
    triangles = vtk.vtkTriangleFilter()
    triangles.SetInputConnection(surface.GetOutputPort())
    triangles.Update()
    mesh = triangles.GetOutput()
    points = vtk_to_numpy(mesh.GetPoints().GetData())[:, :2]
    cells = vtk_to_numpy(mesh.GetPolys().GetData()).reshape(-1, 4)[:, 1:]
    return mesh, points, cells


def interface_lines(mesh, points, cells):
    """由 TriMUSE 的材料编号提取各层轮廓，不跨界面平均场值"""
    attributes = vtk_to_numpy(mesh.GetCellData().GetArray("attribute"))
    rounded = np.round(points, 12)
    lines = set()
    for region in np.unique(attributes):
        edges = Counter()
        for triangle in cells[attributes == region]:
            vertices = [tuple(rounded[index]) for index in triangle]
            for a, b in ((0, 1), (1, 2), (2, 0)):
                edges[tuple(sorted((vertices[a], vertices[b])))] += 1
        lines.update(edge for edge, count in edges.items() if count == 1)
    return np.asarray(sorted(lines))


def sample_comsol(comsol, points, field):
    vtk_points = vtk.vtkPoints()
    vtk_points.SetDataTypeToDouble()
    for x, y in points:
        vtk_points.InsertNextPoint(float(x), float(y), 0.0)
    samples = vtk.vtkPolyData()
    samples.SetPoints(vtk_points)
    probe = vtk.vtkProbeFilter()
    probe.SetInputData(samples)
    probe.SetSourceData(comsol[0])
    probe.Update()
    result = probe.GetOutput().GetPointData()
    valid = vtk_to_numpy(result.GetArray("vtkValidPointMask")).astype(bool)
    values = vtk_to_numpy(result.GetArray(field)).copy()
    values[~valid] = np.nan
    return values


def set_plot_style():
    plt.rcParams.update({"font.family": "Times New Roman", "mathtext.fontset": "stix", "axes.unicode_minus": False, "pdf.fonttype": 42, "ps.fonttype": 42, "font.size": 12, "axes.linewidth": 0.85, "savefig.facecolor": "white"})


def style_axes(ax, title, points):
    coordinates = points * 1e6
    margin = 0.025 * np.max(np.ptp(coordinates, axis=0))
    ax.set_xlim(coordinates[:, 0].min() - margin, coordinates[:, 0].max() + margin)
    ax.set_ylim(coordinates[:, 1].min() - margin, coordinates[:, 1].max() + margin)
    ax.set_aspect("equal")
    ax.set_title(title, fontsize=15, pad=11)
    ax.set_xlabel(r"$r\;(\mu\mathrm{m})$", fontsize=14)
    ax.xaxis.set_major_locator(MaxNLocator(4))
    ax.yaxis.set_major_locator(MaxNLocator(6))
    ax.tick_params(direction="in", top=True, right=True, length=4, width=0.8)
    ax.set_facecolor("#f5f5f5")


def draw_comparison(trimuse, comsol, field, comsol_field, label, destination, unit):
    """应力自动由 Pa 换算为 MPa 展示，差值采用相同换算；原始采样坐标仍为 m"""
    factor = 1e-6 if unit == "Pa" else 1.0
    display_unit = "MPa" if unit == "Pa" else unit
    meshes = (trimuse, comsol)
    values = [vtk_to_numpy(mesh[0].GetPointData().GetArray(name)) * factor for mesh, name in zip(meshes, (field, comsol_field))]
    points, cells = trimuse[1], trimuse[2]
    centers = points[cells].mean(axis=1)
    difference = values[0][cells].mean(axis=1) - sample_comsol(comsol, centers, comsol_field) * factor
    valid = np.isfinite(difference)
    minimum = min(float(np.min(value)) for value in values)
    maximum = max(float(np.max(value)) for value in values)
    if minimum == maximum:
        maximum = minimum + max(abs(minimum) * 1e-6, 1e-6)
    limit = float(np.max(np.abs(difference[valid])))
    difference_limit = limit if limit > 0.0 else 1e-6
    norm = Normalize(minimum, maximum)
    outlines = interface_lines(*trimuse) * 1e6
    fig = plt.figure(figsize=(10.8, 6.9), layout="constrained")
    grid = fig.add_gridspec(2, 3, height_ratios=(1.0, 0.035), hspace=0.10, wspace=0.08)
    axes = [fig.add_subplot(grid[0, i]) for i in range(3)]
    for index, (ax, title, (_, coordinates, triangles), value) in enumerate(zip(axes[:2], ("TriMUSE", "COMSOL"), meshes, values)):
        triangulation = mtri.Triangulation(coordinates[:, 0] * 1e6, coordinates[:, 1] * 1e6, triangles)
        image = ax.tripcolor(triangulation, facecolors=value[triangles].mean(axis=1), shading="flat", cmap="jet", norm=norm, rasterized=True)
        style_axes(ax, f"({chr(97 + index)}) {title}", points)
    error_mesh = mtri.Triangulation(points[:, 0] * 1e6, points[:, 1] * 1e6, cells, mask=~valid)
    error_image = axes[2].tripcolor(error_mesh, facecolors=np.nan_to_num(difference), shading="flat", cmap="RdBu_r", norm=Normalize(-difference_limit, difference_limit), rasterized=True)
    style_axes(axes[2], "(c) TriMUSE − COMSOL", points)
    for index, ax in enumerate(axes):
        ax.add_collection(LineCollection(outlines, colors="#292929", linewidths=0.55, alpha=0.85))
        if index > 0:
            ax.tick_params(labelleft=False)
    axes[0].set_ylabel(r"$z\;(\mu\mathrm{m})$", fontsize=14)
    colorbar = fig.colorbar(image, cax=fig.add_subplot(grid[1, :2]), orientation="horizontal")
    colorbar.set_label(f"{label} [{display_unit}]", fontsize=13, labelpad=4)
    colorbar.locator = MaxNLocator(6)
    colorbar.update_ticks()
    errorbar = fig.colorbar(error_image, cax=fig.add_subplot(grid[1, 2]), orientation="horizontal")
    errorbar.set_label(f"Difference [{display_unit}]", fontsize=13, labelpad=4)
    errorbar.locator = MaxNLocator(3)
    errorbar.update_ticks()
    for bar in (colorbar, errorbar):
        bar.outline.set_linewidth(0.65)
        bar.ax.tick_params(labelsize=10, length=3, width=0.65)
    for extension in ("png", "pdf"):
        fig.savefig(destination / f"{field}_comparison.{extension}", dpi=300)
    plt.close(fig)
    print(f"{field}: max |difference| = {limit:.6g} {display_unit}; valid samples = {valid.sum()}/{valid.size}")


def compare_results(trimuse_directory, comsol_path, fields, destination, cycle=None):
    """通用入口，fields 每项为计算字段、参考字段、标签及输入单位"""
    set_plot_style()
    if cycle is None:
        available = [{path.name for path in (trimuse_directory / field).glob("Cycle*") if path.is_dir()} for field, _, _, _ in fields]
        cycle_name = sorted(set.intersection(*available))[-1]
    else:
        cycle_name = f"Cycle{cycle:06d}"
    destination.mkdir(parents=True, exist_ok=True)
    comsol = read_surface(comsol_path)
    for field, comsol_field, label, unit in fields:
        trimuse = read_surface(trimuse_directory / field / cycle_name / "proc000000.vtu")
        draw_comparison(trimuse, comsol, field, comsol_field, label, destination, unit)
        print(f"{field}: {cycle_name} → {destination}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trimuse-dir", type=Path, required=True, help="包含各字段目录的输出分类目录，例如 output/seed_20261111/mechanics")
    parser.add_argument("--comsol", type=Path, required=True)
    parser.add_argument("--field", nargs=4, action="append", required=True, metavar=("TRIMUSE", "COMSOL", "LABEL", "UNIT"))
    parser.add_argument("--cycle", type=int, help="默认取所有指定字段共有的最后一个输出周期")
    parser.add_argument("--output", type=Path, default=Path("comparison"))
    args = parser.parse_args()
    compare_results(args.trimuse_dir, args.comsol, args.field, args.output, args.cycle)


if __name__ == "__main__":
    main()
