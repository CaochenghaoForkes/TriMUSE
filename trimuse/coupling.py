from __future__ import annotations
from math import ceil
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from _core import OutputManager
    from .input.simulation.config import SimulationCard
    from .problem import Thermal
    from .properties import PropertiesManager

class CouplingManager:
    """管理分阶段时间推进与当前单步场求解"""

    def __init__(self, simulation_config: SimulationCard, properties_manager: PropertiesManager, problems: dict[str, Thermal], output_manager: OutputManager) -> None:
        self.time_span = simulation_config.problem.time_span
        self.min_time_step = simulation_config.problem.min_time_step
        self.write_interval = simulation_config.output.write_interval
        self.steps = simulation_config.coupling.step
        self.field_theta = {field.name: field.theta for field in simulation_config.problem.field}
        self.properties_manager = properties_manager
        self.problems = problems
        self.output_manager = output_manager
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
        self.output_manager.save(self.cycle, self.time, self.time_step)
        print(f"计算进度: 0/{total_steps}, 时间: 0 s", flush=True)
        for duration, time_step, write_interval in zip(self.time_span, self.min_time_step, self.write_interval):
            stage_start = self.time
            step_count = ceil(duration / time_step)
            for step in range(1, step_count + 1):
                next_time = stage_start + min(step * time_step, duration)
                self.time_step = next_time - self.time
                self.time = next_time
                self.cycle += 1
                self.update_and_solve()
                if step % write_interval == 0 or step == step_count:
                    self.output_manager.save(self.cycle, self.time, self.time_step)
                print(f"计算进度: {self.cycle}/{total_steps} ({self.cycle / total_steps:.1%}), 时间: {self.time:.6g} s", flush=True)

    def update_and_solve(self) -> None:
        """更新属性, 按步骤求解场, 然后接受当前时刻"""
        self.properties_manager.update_external(self.time)
        self.properties_manager.update_derived()
        self.properties_manager.update_material()
        for step in self.steps:
            if step.method != "single":
                raise NotImplementedError("交错耦合迭代尚未实现")
            for field_name in step.fields:
                problem = self.problems[field_name]
                theta = self.field_theta[field_name]
                problem.solve(self.time_step, 1.0 if theta is None else theta)
                self.properties_manager.update_field_dependent_materials(problem.field_state)
        self.properties_manager.accept_current_state()
