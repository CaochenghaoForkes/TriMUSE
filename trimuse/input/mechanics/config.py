from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from typing import cast
from xml.etree import ElementTree as ET
import sys

from ..xml_utils import XmlUtils

@dataclass
class MechanicsCard:
    """力学参考温度、外表面边界、内部材料界面和体积力配置"""

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

    @dataclass
    class Source:
        """按材料区域配置体积力"""

        @dataclass
        class Region:
            name: str
            model: str
            parameters: dict[str, float | list[float]] = field(default_factory=dict)

        region: list[Region] = field(default_factory=list)

    thermal_expansion_reference_temperature: float
    boundary: list[Boundary] = field(default_factory=list)
    interfaces: Interfaces = field(default_factory=Interfaces)
    source: Source = field(default_factory=Source)

    def precomputed_parameters(self) -> dict[str, str | float]:
        return {"thermal_expansion_reference_temperature": self.thermal_expansion_reference_temperature}

class MechanicsCardLoader:
    @classmethod
    def load(cls, path: str | Path | None = None) -> MechanicsCard:
        """读取力学配置，未指定的外边界为零面力，未指定的界面为完美粘结"""
        if path is None or isinstance(path, str) and not path.strip():
            template_path = XmlUtils.write_template(Path(__file__).with_name("mechanics.xml"), "mechanics_empty.xml")
            print(f"力学模板输入卡已生成: {template_path}")
            sys.exit()
        root = XmlUtils.get_root(path, "mechanics")
        temperature = cls.read_reference_temperature(root)
        boundaries = XmlUtils.read_child(root, "boundaries", required=False)
        boundary = [cls.read_boundary(element) for element in XmlUtils.read_children(boundaries, "boundary")] if boundaries is not None else []
        interfaces = cls.read_interfaces(XmlUtils.read_child(root, "interfaces", required=False))
        source = cls.read_source(root)
        return MechanicsCard(thermal_expansion_reference_temperature=temperature, boundary=boundary, interfaces=interfaces, source=source)

    @staticmethod
    def read_reference_temperature(root: ET.Element) -> float:
        name = "thermal_expansion_reference_temperature"
        parameters = XmlUtils.read_problem_parameters(root, {name: "temperature"})
        if parameters is not None:
            if XmlUtils.read_child(root, name, required=False) is not None:
                raise ValueError(f"mechanics: 请仅使用 <parameters> 或旧版 <{name}> 中的一种")
            return cast(float, parameters[name])
        legacy = XmlUtils.read_child(root, name)
        return cast(float, XmlUtils.read_attribute(legacy, "value", float, required=True, quantity="temperature"))

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

    @classmethod
    def read_source(cls, root: ET.Element) -> MechanicsCard.Source:
        source = XmlUtils.read_child(root, "source", required=False)
        legacy = XmlUtils.read_child(root, "body_forces", required=False)
        if source is not None and legacy is not None:
            raise ValueError("mechanics: 请仅使用 <source> 或旧版 <body_forces> 中的一种")
        if source is not None:
            regions = [cls.read_source_region(element) for element in XmlUtils.read_children(source, "region")]
        elif legacy is not None:
            regions = [cls.read_source_region(element, "region") for element in XmlUtils.read_children(legacy, "body_force")]
        else:
            regions = []
        return MechanicsCard.Source(region=regions)

    @staticmethod
    def read_source_region(element: ET.Element, name_attribute: str = "name") -> MechanicsCard.Source.Region:
        name = XmlUtils.read_attribute(element, name_attribute, required=True)
        model_config = XmlUtils.read_model(element, context=f"source.{name}")
        return MechanicsCard.Source.Region(name=name, **model_config)
