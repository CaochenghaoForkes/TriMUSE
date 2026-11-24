from __future__ import annotations
from typing import TYPE_CHECKING

from _core import FemSystem

if TYPE_CHECKING:
    from .geometry.topology import MeshInfo
    from .input.xml_loader import InputCards

class FemManager:
    """管理公共网格 求积规则与各场有限元对象"""

    def __init__(self, input_cards: InputCards, mesh_info: MeshInfo) -> None:
        self.mesh_info = mesh_info
        solver_config = input_cards.simulation.solver
        quadrature = solver_config.quadrature
        self.fem_system = FemSystem(
            str(mesh_info.path),
            domain_quadrature_order=quadrature.domain_order,
            boundary_quadrature_order=quadrature.boundary_order,
            geometry_method=mesh_info.geometry_method,
            center=mesh_info.center,
            region_layers=mesh_info.region_layers,
        )
        for field in solver_config.field:
            dimension = mesh_info.dimension if field.name == "displacement" else 1
            self.fem_system.add_region_field(field.name, order=field.fem_space.order, vector_dimension=dimension)
            print(f"有限元场已创建: {field.name}, 阶次: {field.fem_space.order}, 分域 H1")
