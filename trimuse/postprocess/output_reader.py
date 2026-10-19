from __future__ import annotations
from collections.abc import Iterator
from pathlib import Path
from typing import TYPE_CHECKING
from xml.etree import ElementTree as ET

from vtkmodules.vtkIOXML import vtkXMLPUnstructuredGridReader, vtkXMLUnstructuredGridReader

if TYPE_CHECKING:
    from vtkmodules.vtkCommonDataModel import vtkUnstructuredGrid

class OutputReader:
    """读取属性时间序列索引, 按需加载各帧, 不一次缓存全部场数据"""

    property_field_map = {"temperature": "thermal"}

    def __init__(self, output_directory: str | Path, properties: list[str]) -> None:
        """output_directory 为包含 seed 子目录后实际使用的输出根目录"""
        self.directory = Path(output_directory).expanduser().resolve()
        self.series: dict[str, list[tuple[float, Path]]] = {}
        for name in properties:
            field = self.property_field_map[name]
            path = self.directory / field / name / f"{name}.pvd"
            root = ET.parse(path).getroot()
            self.series[name] = sorted(
                (float(item.get("timestep")), (path.parent / item.get("file")).resolve())
                for item in root.findall("./Collection/DataSet")
            )

    def times(self, property_name: str) -> list[float]:
        """返回该属性全部已保存的实际模拟时间, 单位 s"""
        return [time for time, _ in self.series[property_name]]

    def read_frame(self, property_name: str, index: int) -> vtkUnstructuredGrid:
        """读取一帧完整网格与数据; PVTU 自动汇集其引用的各个 VTU 分片"""
        _, path = self.series[property_name][index]
        if not path.is_file():
            raise FileNotFoundError(path)
        readers = {".pvtu": vtkXMLPUnstructuredGridReader, ".vtu": vtkXMLUnstructuredGridReader}
        reader = readers[path.suffix.lower()]()
        reader.SetFileName(str(path))
        errors = []
        reader.AddObserver("ErrorEvent", lambda *_: errors.append(True))
        reader.Update()
        if errors or reader.GetErrorCode() != 0:
            raise RuntimeError(f"无法读取场输出: {path}")
        mesh = reader.GetOutput()
        if mesh.GetPointData().GetArray(property_name) is None and mesh.GetCellData().GetArray(property_name) is None:
            raise ValueError(f"输出帧中缺少属性 {property_name}: {path}")
        return mesh

    def iter_frames(self, property_name: str) -> Iterator[tuple[float, vtkUnstructuredGrid]]:
        """按时间顺序逐帧返回 (模拟时间, 网格与场数据)"""
        for index, (time, _) in enumerate(self.series[property_name]):
            yield time, self.read_frame(property_name, index)
