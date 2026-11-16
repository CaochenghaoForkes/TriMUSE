from __future__ import annotations
from math import ceil, floor, isclose
from typing import TYPE_CHECKING
from .problem import Mechanics, Radioactivity, Thermal

if TYPE_CHECKING:
    from .fem import FemManager
    from .input.xml_loader import InputCards
    from .output import OutputManager
    from .properties import PropertiesManager
    from .solver import SolverManager

PROBLEM_TYPES = {
    "temperature": Thermal,
    "concentration": Radioactivity,
    "displacement": Mechanics,
}

class CouplingManager:
    """管理分阶段时间推进与当前单步场求解"""

    def __init__(self, input_cards: InputCards, fem_manager: FemManager, properties_manager: PropertiesManager, solver_manager: SolverManager, output_manager: OutputManager) -> None:
        simulation_config = input_cards.simulation
        fields = simulation_config.problem.field
        self.time_span = simulation_config.problem.time_span
        self.min_time_step = simulation_config.problem.min_time_step
        self.write_interval = simulation_config.output.write_interval
        self.steps = simulation_config.coupling.step
        self.field_theta = {field.name: field.theta for field in fields}
        self.properties_manager = properties_manager
        self.output_manager = output_manager
        self.output_manager.initialize_field_output(fem_manager)
        self.problems = {field.name: PROBLEM_TYPES[field.name](input_cards, fem_manager, properties_manager, solver_manager, output_manager) for field in fields}
        self.cycle = 0
        self.time = 0.0
        self.time_step = 0.0

    def run(self) -> None:
        """从初始时刻推进, 各阶段最后一步截到阶段终点"""
        self.time = 0.0
        self.time_step = 0.0
        self.cycle = 0
        total_steps = sum(ceil(duration / time_step) for duration, time_step in zip(self.time_span, self.min_time_step))
        self.update_and_solve()
        self.output_manager.save(self.cycle, self.time, self.time_step, self.problems)
        print(f"计算进度: 0/{total_steps}, 时间: 0 s", flush=True)
        for duration, time_step, write_interval in zip(self.time_span, self.min_time_step, self.write_interval):
            stage_start = self.time
            step_count = ceil(duration / time_step)
            next_output = 1 
            for step in range(1, step_count + 1):
                next_time = stage_start + min(step * time_step, duration)
                self.time_step = next_time - self.time
                self.time = next_time
                self.cycle += 1
                self.update_and_solve()
                if write_interval < 0:
                    write_due = step % int(-write_interval) == 0
                else:
                    elapsed = min(step * time_step, duration)
                    output_period = floor(elapsed / write_interval)
                    if isclose(elapsed, (output_period + 1) * write_interval, rel_tol=1e-12, abs_tol=0.0):
                        output_period += 1
                    write_due = output_period >= next_output
                    if write_due:
                        next_output = output_period + 1
                if write_due or step == step_count:
                    self.output_manager.save(self.cycle, self.time, self.time_step, self.problems)
                print(f"计算进度: {self.cycle}/{total_steps} ({self.cycle / total_steps:.1%}), 时间: {self.time:.6g} s", flush=True)
        self.output_manager.finish_simulation(self.time, self.cycle)

    def update_and_solve(self) -> None:
        """更新属性, 按步骤求解场, 然后接受当前时刻"""
        self.properties_manager.update_external(self.time, self.time_step)
        self.properties_manager.update_derived()
        self.properties_manager.update_material()
        for step in self.steps:
            for field_name in step.fields:
                problem = self.problems[field_name]
                theta = self.field_theta[field_name]
                problem.solve(self.time_step, 1.0 if theta is None else theta)
                self.output_manager.save_solve_information(field_name, self.cycle, self.time, self.time_step)
                self.properties_manager.update_field_dependent_materials(problem.field_state)
        self.properties_manager.accept_current_state()
