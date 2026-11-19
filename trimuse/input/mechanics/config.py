from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from xml.etree import ElementTree as ET
import sys

from ..xml_utils import ProblemParameters, XmlUtils

@dataclass
class MechanicsCard:
    """按 XML 层级保存力学问题配置，位置名称保持原始字符串"""

    @dataclass
    class Model:
        name: str
        model_name: str
        parameters: ProblemParameters = field(default_factory=dict)

    @dataclass
    class Regions:
        @dataclass
        class Region:
            name: str
            model: list[MechanicsCard.Model] = field(default_factory=list)

        region: list[Region] = field(default_factory=list)

    @dataclass
    class Boundaries:
        @dataclass
        class Boundary:
            name: str
            model: list[MechanicsCard.Model] = field(default_factory=list)

        boundary: list[Boundary] = field(default_factory=list)

    @dataclass
    class Interfaces:
        @dataclass
        class Interface:
            name: str
            model: list[MechanicsCard.Model] = field(default_factory=list)

        interface: list[Interface] = field(default_factory=list)

    parameters: ProblemParameters = field(default_factory=dict)
    regions: Regions = field(default_factory=Regions)
    boundaries: Boundaries = field(default_factory=Boundaries)
    interfaces: Interfaces = field(default_factory=Interfaces)

class MechanicsCardLoader:
    @classmethod
    def load(cls, path: str | Path | None = None) -> MechanicsCard:
        """读取原始输入结构，空路径生成模板并退出"""
        if path is None or isinstance(path, str) and not path.strip():
            template_path = XmlUtils.write_template(Path(__file__).with_name("mechanics.xml"), "mechanics_empty.xml")
            print(f"力学模板输入卡已生成: {template_path}")
            sys.exit()
        root = XmlUtils.get_root(path, "mechanics")
        XmlUtils.check_children(root, ('parameters', 'regions', 'boundaries', 'interfaces'), context="mechanics")
        parameters = XmlUtils.read_problem_parameters(root)
        regions = cls.read_regions(XmlUtils.read_child(root, "regions"))
        boundaries = cls.read_boundaries(XmlUtils.read_child(root, "boundaries", required=False))
        interfaces = cls.read_interfaces(XmlUtils.read_child(root, "interfaces", required=False))
        return MechanicsCard(parameters, regions, boundaries, interfaces)

    @classmethod
    def read_regions(cls, element: ET.Element) -> MechanicsCard.Regions:
        regions = MechanicsCard.Regions()
        XmlUtils.check_children(element, ("region",), context="mechanics.regions")
        for child in XmlUtils.read_children(element, "region", required=True):
            XmlUtils.check_attributes(child, ("name",))
            name = XmlUtils.read_attribute(child, "name", required=True)
            regions.region.append(MechanicsCard.Regions.Region(name=name, model=cls.read_models(child)))
        return regions

    @classmethod
    def read_boundaries(cls, element: ET.Element | None) -> MechanicsCard.Boundaries:
        boundaries = MechanicsCard.Boundaries()
        if element is None:
            return boundaries
        XmlUtils.check_children(element, ("boundary",), context="mechanics.boundaries")
        for child in XmlUtils.read_children(element, "boundary", required=False):
            XmlUtils.check_attributes(child, ("name",))
            name = XmlUtils.read_attribute(child, "name", required=True)
            boundaries.boundary.append(MechanicsCard.Boundaries.Boundary(name=name, model=cls.read_models(child)))
        return boundaries

    @classmethod
    def read_interfaces(cls, element: ET.Element | None) -> MechanicsCard.Interfaces:
        interfaces = MechanicsCard.Interfaces()
        if element is None:
            return interfaces
        XmlUtils.check_children(element, ("interface",), context="mechanics.interfaces")
        for child in XmlUtils.read_children(element, "interface", required=False):
            XmlUtils.check_attributes(child, ("name",))
            name = XmlUtils.read_attribute(child, "name", required=True)
            interfaces.interface.append(MechanicsCard.Interfaces.Interface(name=name, model=cls.read_models(child)))
        return interfaces

    @staticmethod
    def read_models(element: ET.Element) -> list[MechanicsCard.Model]:
        XmlUtils.check_children(element, ("model",))
        models = []
        for child in XmlUtils.read_children(element, "model", required=False):
            XmlUtils.check_attributes(child, ("name", "model_name"))
            XmlUtils.check_children(child, ("parameter",))
            name = XmlUtils.read_attribute(child, "name", required=True)
            model_name = XmlUtils.read_attribute(child, "model_name", required=True)
            parameters = XmlUtils.read_parameters(child, f"mechanics.{name}.{model_name}")
            models.append(MechanicsCard.Model(name=name, model_name=model_name, parameters=parameters))
        return models
