from __future__ import annotations
from collections import Counter
from datetime import datetime
from pathlib import Path
from time import perf_counter
from typing import TYPE_CHECKING
from zoneinfo import ZoneInfo
import json


if TYPE_CHECKING:
    from .fem import FemManager
    from .properties import PropertiesManager
    from .input.xml_loader import InputCards

class OutputManager:
    """统一管理输出目录、C++ 场输出和模拟完成信息"""

    def __init__(self, input_cards: InputCards) -> None:
        self.input_cards = input_cards
        self.directory = Path(input_cards.chief.output.directory).expanduser().resolve()
        suffix = 2
        while self.directory.exists():
            self.directory = self.directory.parent.with_name(f"{self.directory.parent.name}_{suffix}") / self.directory.name
            suffix += 1
        self.directory.mkdir(parents=True)
        input_cards.chief.output.directory = str(self.directory)
        self.report_file = self.directory / "simulation_report.txt"
        self.started_at = self._now()
        self.start_clock = perf_counter()
        self.mesh_statistics = None
        self.frame_count = 0
        self.field_output = None

    @staticmethod
    def _now() -> str:
        return datetime.now(ZoneInfo("Asia/Shanghai")).isoformat(timespec="seconds")

    def record_mesh(self, mesh_path: str | Path) -> None:
        """统计实际导出的原始网格, 不统计后处理细分单元; 总数包含各维数单元"""
        from vtkmodules.vtkCommonDataModel import vtkCellTypes
        from vtkmodules.vtkIOLegacy import vtkUnstructuredGridReader

        reader = vtkUnstructuredGridReader()
        reader.SetFileName(str(Path(mesh_path).with_suffix(".vtk")))
        reader.Update()
        mesh = reader.GetOutput()
        counts = Counter(mesh.GetCellType(index) for index in range(mesh.GetNumberOfCells()))
        dimension = self.input_cards.geometry_mesh.model.dimension
        self.mesh_statistics = {
            "total_element_count": sum(counts.values()),
            "domain_element_count": sum(count for cell_type, count in counts.items() if vtkCellTypes.GetDimension(cell_type) == dimension),
            "boundary_element_count": sum(count for cell_type, count in counts.items() if vtkCellTypes.GetDimension(cell_type) == dimension - 1)
        }

    def initialize_field_output(self, fem_manager: FemManager, properties_manager: PropertiesManager) -> None:
        """记录网格统计，创建 C++ 输出管理器并初始化运行报告"""
        from _core import OutputManager as CoreOutputManager
        self.record_mesh(fem_manager.mesh_info.path)
        self.field_output = CoreOutputManager(fem_manager.fem_system, str(self.directory))
        directories = {"temperature": "thermal", "concentration": "radioactivity", "von_mises_stress": "mechanics", "max_principal_stress": "mechanics"}
        configs = {"domain": [], "scalar": []}
        for name in self.input_cards.simulation.output.domain_properties:
            configs["domain"].append((name, directories.get(name, "properties")))
        for name in self.input_cards.simulation.output.scalar_properties:
            configs["scalar"].append((name, "scalar_properties"))
        location_names = {"region": {}, "zone": {}}
        for name, attribute in fem_manager.mesh_info.region_attributes.items():
            location_names["region"][attribute] = name
        for name, attribute in fem_manager.mesh_info.boundary_attributes.items():
            location_names["zone"][attribute] = name
        self.field_output.initialize(properties_manager.properties_system, configs, location_names)
        self.report_file.write_text("TriMUSE simulation report\n", encoding="utf-8")

    def save(self, cycle: int, time: float, time_step: float) -> None:
        self.field_output.save(cycle, time, time_step)
        self.frame_count += 1

    def _append_report(self, text: str) -> None:
        with self.report_file.open("a", encoding="utf-8") as report:
            report.write(text)

    def save_solve_information(self, field_name: str, cycle: int, time: float, time_step: float) -> None:
        """每次场求解后写入报告, 保留各轮 CG 的重复条目及原始顺序"""
        information = self.field_output.solve_information(field_name)
        if not information: # 瞬态初始时刻不进行求解
            return
        header = f"\n[solve] field={field_name}, cycle={cycle}, time_s={time:.12g}, time_step_s={time_step:.12g}\n"
        self._append_report(header + "".join(f"  {name}: {value:.12g}\n" for name, value in information))

    def finish_simulation(self, simulation_time: float, step_count: int) -> None:
        """追加模拟概况, 最后一行写入完成标记; 此后不再追加内容"""
        cards = self.input_cards
        info = {
            "random_seed": cards.chief.random_seed,
            "dimension": cards.geometry_mesh.model.dimension,
            "mesh": self.mesh_statistics,
            "fields": [item.name for item in cards.simulation.problem.field],
            "simulation_time_s": simulation_time,
            "time_steps": step_count,
            "output_frames": self.frame_count,
            "started_at": self.started_at,
            "finished_at": self._now(),
            "elapsed_seconds": perf_counter() - self.start_clock
        }
        summary = json.dumps(info, ensure_ascii=False, indent=2) + "\n"
        self._append_report("\n[simulation_summary]\n" + summary + "simulation_completed\n")
