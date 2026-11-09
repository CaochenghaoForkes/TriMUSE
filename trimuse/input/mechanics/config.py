from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from xml.etree import ElementTree as ET
import sys

from ..xml_utils import XmlUtils

@dataclass
class MechanicsCard:
    """力学参考温度、外表面边界和内部材料界面配置"""

    @dataclass
    class Boundary:
        name: str
        model: str
        parameters: dict[str, float | list[float]] = field(default_factory=dict)

    @dataclass
    class Interfaces:
        @dataclass
        class Interface:
            name: str
            model: str
            parameters: dict[str, float | list[float]] = field(default_factory=dict)

        interface: list[Interface] = field(default_factory=list)

    thermal_expansion_reference_temperature: float
    boundary: list[Boundary] = field(default_factory=list)
    interfaces: Interfaces = field(default_factory=Interfaces)

class MechanicsCardLoader:
    @classmethod
    def load(cls, path: str | Path | None = None) -> MechanicsCard:
        """读取力学配置，未指定的外边界为零面力，未指定的界面为完美粘结"""
        if path is None or isinstance(path, str) and not path.strip():
            template_path = XmlUtils.write_template(Path(__file__).with_name("mechanics.xml"), "mechanics_empty.xml")
            print(f"力学模板输入卡已生成: {template_path}")
            sys.exit()
        root = XmlUtils.get_root(path, "mechanics")
        reference = XmlUtils.read_child(root, "thermal_expansion_reference_temperature")
        temperature = XmlUtils.read_attribute(reference, "value", float, required=True, quantity="temperature")
        boundaries = XmlUtils.read_child(root, "boundaries", required=False)
        boundary = [cls.read_boundary(element) for element in XmlUtils.read_children(boundaries, "boundary")] if boundaries is not None else []
        interfaces = cls.read_interfaces(XmlUtils.read_child(root, "interfaces", required=False))
        return MechanicsCard(thermal_expansion_reference_temperature=temperature, boundary=boundary, interfaces=interfaces)

    @staticmethod
    def read_boundary(element: ET.Element) -> MechanicsCard.Boundary:
        name = XmlUtils.read_attribute(element, "name", required=True)
        model_config = XmlUtils.read_model(element, context=f"boundary.{name}")
        return MechanicsCard.Boundary(name=name, **model_config)

    @classmethod
    def read_interfaces(cls, element: ET.Element | None) -> MechanicsCard.Interfaces:
        interfaces = [cls.read_interface(child) for child in XmlUtils.read_children(element, "interface")] if element is not None else []
        return MechanicsCard.Interfaces(interface=interfaces)

    @staticmethod
    def read_interface(element: ET.Element) -> MechanicsCard.Interfaces.Interface:
        name = XmlUtils.read_attribute(element, "name", required=True)
        model_config = XmlUtils.read_model(element, context=f"interfaces.{name}")
        return MechanicsCard.Interfaces.Interface(name=name, **model_config)
