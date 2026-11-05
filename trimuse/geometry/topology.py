from pathlib import Path
from dataclasses import dataclass, field

LayerParameters = dict[str, float | list[float]]
RegionLayers = dict[str, LayerParameters]  # inner、outer；kernel 的 inner 为空，表示半径零


def make_region_layers(names: list[str], profiles: list[LayerParameters]) -> dict[str, RegionLayers]:
    """按从内到外的实际轮廓参数，为各材料区域登记内、外表面"""
    return {name: {"inner": profiles[index - 1] if index else {}, "outer": profiles[index]} for index, name in enumerate(names)}


@dataclass
class GeometryTopology:
    """保存物理名称、Gmsh 实体 tag 以及各区域内外层的实际构造参数"""

    dimension: int
    regions: dict[str, list[int]] = field(default_factory=dict)
    boundaries: dict[str, list[int]] = field(default_factory=dict)
    region_layers: dict[str, RegionLayers] = field(default_factory=dict)

@dataclass
class MeshInfo:
    """保存网格文件、物理属性映射与 FEM 材料方向计算所需的几何描述"""

    path: Path
    dimension: int
    region_attributes: dict[str, int]
    boundary_attributes: dict[str, int]
    geometry_method: str = ""
    center: tuple[float, float, float] = (0.0, 0.0, 0.0)
    region_layers: dict[int, RegionLayers] = field(default_factory=dict)
