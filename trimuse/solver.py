from __future__ import annotations
from typing import TYPE_CHECKING

from _core import SolverSystem # type: ignore

if TYPE_CHECKING:
    from .input.xml_loader import InputCards

class SolverManager:
    """按场整理求解配置并创建 C++ 求解系统"""

    def __init__(self, input_cards: InputCards) -> None:
        config = input_cards.simulation.solver
        entries = [
            (
                field.name,
                (field.linear_solver.method, field.linear_solver.preconditioner,
                 field.linear_solver.relative_tolerance, field.linear_solver.max_iterations),
                (field.nonlinear_solver.method, field.nonlinear_solver.relative_tolerance,
                 field.nonlinear_solver.absolute_tolerance, field.nonlinear_solver.max_iterations)
            )
            for field in config.field
        ]
        self.solver_system = SolverSystem(entries)
