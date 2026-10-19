from pathlib import Path
from dataclasses import dataclass, field

@dataclass
class GeometryTopology:
    """保存物理名称与 Gmsh 几何实体 tag 的对应关系"""

    dimension: int
    regions: dict[str, list[int]] = field(default_factory=dict)
    boundaries: dict[str, list[int]] = field(default_factory=dict)

@dataclass
class MeshInfo:
    """保存生成后的网格文件及物理属性映射"""

    path: Path
    dimension: int
    region_attributes: dict[str, int]
    boundary_attributes: dict[str, int]