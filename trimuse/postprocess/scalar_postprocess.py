from __future__ import annotations
from pathlib import Path
from typing import TYPE_CHECKING
import csv
import json
import shutil
import numpy as np

from .plot_style import plot_style, property_label, scientific_axes

if TYPE_CHECKING:
    from trimuse.input.xml_loader import InputCards

class ScalarPostprocessManager:
    """读取标量 CSV，每项属性绘制各位置随时间变化的曲线"""

    def __init__(self, input_cards: InputCards) -> None:
        self.names = input_cards.postprocess.scalar_properties.names
        self.formats = set(input_cards.postprocess.output.formats)
        self.output_directory = Path(input_cards.chief.output.directory).expanduser().resolve()
        self.postprocess_directory = self.output_directory / "postprocess" / "scalar_properties"
        self.units = json.loads((self.output_directory / "property_units.json").read_text(encoding="utf-8"))

    def run(self) -> None:
        if not self.formats.intersection({"png", "csv"}):
            return
        self.postprocess_directory.mkdir(parents=True, exist_ok=True)
        for name in self.names:
            path = self.output_directory / "scalar_properties" / f"{name}.csv"
            if "csv" in self.formats:
                shutil.copyfile(path, self.postprocess_directory / path.name)
            if "png" in self.formats:
                with path.open(encoding="utf-8", newline="") as stream:
                    columns = next(csv.reader(stream))
                    data = np.genfromtxt(stream, delimiter=",", ndmin=2)
                self.plot(name, columns, data)

    @plot_style
    def plot(self, name: str, columns: list[str], data: np.ndarray) -> Path:
        """前三列为 cycle、time、time_step，其余每列对应一条位置曲线"""
        from matplotlib.figure import Figure
        from matplotlib.backends.backend_agg import FigureCanvasAgg

        figure = Figure()
        FigureCanvasAgg(figure)
        axis = figure.subplots()
        for index, location in enumerate(columns[3:], start=3):
            axis.plot(data[:, 1], data[:, index], label=location)
        axis.set_xlabel("Time (s)", fontstyle="italic")
        axis.set_ylabel(property_label(name, {name: {"unit": self.units[name]}}), fontstyle="italic")
        axis.legend()
        scientific_axes(axis)
        figure.tight_layout()
        path = self.postprocess_directory / f"{name}.png"
        figure.savefig(path)
        return path
