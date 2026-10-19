from __future__ import annotations
from typing import TYPE_CHECKING

from _core import FemSystem # type: ignore

if TYPE_CHECKING:
    from .geometry.topology import MeshInfo
    from .input.simulation.config import SimulationCard

class FemManager:
    """管理公共网格 求积规则与各场有限元对象"""

    def __init__(self, mesh_info: MeshInfo, solver_config: SimulationCard.Solver) -> None:
        quadrature = solver_config.quadrature
        self.fem_system = FemSystem(
            str(mesh_info.path),
            domain_quadrature_order=quadrature.domain_order,
            boundary_quadrature_order=quadrature.boundary_order
        )
        for field in solver_config.field:
            self.fem_system.add_field(field.name, order=field.fem_space.order, vector_dimension=1)
            print(f"有限元场已创建: {field.name}, 阶次: {field.fem_space.order}")
