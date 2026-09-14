"""不同几何构造器共用的几何拓扑数据"""

from dataclasses import dataclass, field

@dataclass
class GeometryTopology:
    """保存物理名称与 Gmsh 几何实体 tag 的对应关系"""

    dimension: int
    regions: dict[str, list[int]] = field(default_factory=dict)
    boundaries: dict[str, list[int]] = field(default_factory=dict)
