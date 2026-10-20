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

    property_field_map = {"temperature": "thermal"} # 每个属性的分类 用于寻找其在哪个目录下

    def __init__(self, output_directory: str | Path, properties: list[str]) -> None:
        """output_directory 为包含 seed 子目录后实际使用的输出根目录"""
        self.directory = Path(output_directory).expanduser().resolve() # expanduser() 若传入包含“~”则展开为系统当前用户主目录 resolve() 转为规范的绝对路径
        self.series: dict[str, list[tuple[float, Path]]] = {} # 每个属性对应的“时间—数据文件路径”列表
        for name in properties:
            field = self.property_field_map[name]
            path = self.directory / field / name / f"{name}.pvd"
            root = ET.parse(path).getroot()
            self.series[name] = sorted((float(item.get("timestep")), (path.parent / item.get("file")).resolve()) for item in root.findall("./Collection/DataSet")) # pvd 文件中所有时间步与其对应的子 pvtu 文件的路径

    def times(self, property_name: str) -> list[float]:
        """返回该属性全部已保存的实际模拟时间, 单位 s"""
        return [time for time, _ in self.series[property_name]]

    def read_frame(self, property_name: str, index: int) -> vtkUnstructuredGrid:
        """读取一帧完整网格与数据; PVTU 自动汇集其引用的各个 VTU 分片"""
        _, path = self.series[property_name][index]
        if not path.is_file():
            raise FileNotFoundError(path)
        readers = {".pvtu": vtkXMLPUnstructuredGridReader, ".vtu": vtkXMLUnstructuredGridReader} # .pvtu 它引用并汇集了多个网格分片 # .vtu 非结构网格文件
        reader = readers[path.suffix.lower()]() # 根据 path 后缀找到对应读取器
        reader.SetFileName(str(path)) # 设置要读取的文件路径
        errors = [] # 记录是否发生错误
        reader.AddObserver("ErrorEvent", lambda *_: errors.append(True)) # 如果 VTK 发出 ErrorEvent 就执行 errors.append(True)
        reader.Update() # 执行读取，对于 pvtu 他还会读其中引用的 .vtu 分片，形成网格及场数据
        if errors or reader.GetErrorCode() != 0:
            raise RuntimeError(f"无法读取场输出: {path}")
        mesh = reader.GetOutput() # 读取出的 VTK 网格对象 它不仅包含几何和单元连接关系，也包含温度等数据
        if mesh.GetPointData().GetArray(property_name) is None and mesh.GetCellData().GetArray(property_name) is None: # 在节点数据与单元数据中读取属性名对应的值，如果都读不到就报错
            raise ValueError(f"输出帧中缺少属性 {property_name}: {path}")
        return mesh

    def iter_frames(self, property_name: str) -> Iterator[tuple[float, vtkUnstructuredGrid]]:
        """按时间顺序逐帧返回 (模拟时间, 网格与场数据)"""
        for index, (time, _) in enumerate(self.series[property_name]):
            yield time, self.read_frame(property_name, index) # yield 会暂停函数，而不是结束它；下一次取值时，继续读下一帧
