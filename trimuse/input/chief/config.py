from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from xml.etree import ElementTree as ET
import sys

from ..xml_utils import XmlUtils
from .template import ChiefCardTemplate

@dataclass
class ChiefCard:
    """总调度输入卡配置, 输入路径和输出根目录相对于总调度卡解析"""

    @dataclass
    class InputFiles:
        """各模块输入卡路径, 空路径用于请求生成对应模板"""

        geometry_mesh: str | None = None
        material: str | None = None
        properties: str | None = None
        thermal: str | None = None
        simulation: str | None = None
        postprocess: str | None = None

    @dataclass
    class Output:
        """实际输出根目录: 用户指定目录/seed_<随机种子>"""

        directory: str

    name: str
    random_seed: int
    output: Output
    input_files: InputFiles = field(default_factory=InputFiles)

class ChiefCardLoader:
    """读取总调度配置, 空路径在当前工作目录生成模板并退出"""

    @classmethod
    def load(cls, path: str | Path | None = None) -> ChiefCard:
        """路径为空时生成 chief_empty.xml, 否则读取总调度卡"""
        if path is None or isinstance(path, str) and not path.strip():
            template_path = ChiefCardTemplate.write()
            print(f"主模板输入卡已生成: {template_path}")
            sys.exit()
        chief_path = Path(path).expanduser().resolve()
        root = XmlUtils.get_root(chief_path, "trimuse_chief")
        name = XmlUtils.read_attribute(root, "name", required=True)
        random_seed = cls.read_random(XmlUtils.read_child(root, "random"))
        output = cls.read_output(XmlUtils.read_child(root, "output"), chief_path.parent, random_seed)
        input_files = cls.read_input_files(XmlUtils.read_child(root, "input_files"), chief_path.parent)
        chief_card = ChiefCard(
            name=name,
            random_seed=random_seed,
            output=output,
            input_files=input_files
        )
        return chief_card

    @staticmethod
    def read_random(element: ET.Element) -> int:
        """读取整个算例共用的随机种子"""
        random_seed = XmlUtils.read_attribute(element, "seed", int, required=True)
        return random_seed

    @staticmethod
    def read_output(element: ET.Element, base_directory: Path, random_seed: int) -> ChiefCard.Output:
        """解析用户目录并追加种子子目录, 各模块共用此实际输出根目录"""
        directory = XmlUtils.read_attribute(element, "directory", required=True)
        directory = str(XmlUtils.resolve_path(directory, base_directory) / f"seed_{random_seed}")
        output = ChiefCard.Output(directory=directory)
        return output

    @staticmethod
    def read_input_files(element: ET.Element, base_directory: Path) -> ChiefCard.InputFiles:
        """解析子卡路径, 未填写的路径保留为 None"""
        geometry_mesh = XmlUtils.read_child(element, "geometry_mesh", required=False)
        geometry_mesh_path = XmlUtils.read_attribute(geometry_mesh, "file", default="", allow_empty=True)
        geometry_mesh_path = str(XmlUtils.resolve_path(geometry_mesh_path, base_directory)) if geometry_mesh_path else None
        material = XmlUtils.read_child(element, "material", required=False)
        material_path = XmlUtils.read_attribute(material, "file", default="", allow_empty=True)
        material_path = str(XmlUtils.resolve_path(material_path, base_directory)) if material_path else None
        properties = XmlUtils.read_child(element, "properties", required=False)
        properties_path = XmlUtils.read_attribute(properties, "file", default="", allow_empty=True)
        properties_path = str(XmlUtils.resolve_path(properties_path, base_directory)) if properties_path else None
        thermal = XmlUtils.read_child(element, "thermal", required=False)
        thermal_path = XmlUtils.read_attribute(thermal, "file", default="", allow_empty=True)
        thermal_path = str(XmlUtils.resolve_path(thermal_path, base_directory)) if thermal_path else None
        simulation = XmlUtils.read_child(element, "simulation", required=False)
        simulation_path = XmlUtils.read_attribute(simulation, "file", default="", allow_empty=True)
        simulation_path = str(XmlUtils.resolve_path(simulation_path, base_directory)) if simulation_path else None
        postprocess = XmlUtils.read_child(element, "postprocess", required=False)
        postprocess_path = XmlUtils.read_attribute(postprocess, "file", default="", allow_empty=True)
        postprocess_path = str(XmlUtils.resolve_path(postprocess_path, base_directory)) if postprocess_path else None
        input_files = ChiefCard.InputFiles(geometry_mesh=geometry_mesh_path, material=material_path, properties=properties_path, thermal=thermal_path, simulation=simulation_path, postprocess=postprocess_path)
        return input_files
