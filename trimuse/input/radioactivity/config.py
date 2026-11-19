from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from xml.etree import ElementTree as ET
import sys

from ..xml_utils import ProblemParameters, XmlUtils

@dataclass
class RadioactivityCard:
    """按 XML 层级保存放射性问题配置，位置名称保持原始字符串"""

    @dataclass
    class Initial:
        @dataclass
        class Concentration:
            region: str
            model: str
            parameters: dict[str, float | list[float]] = field(default_factory=dict)

        concentration: list[Concentration] = field(default_factory=list)

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
            model: list[RadioactivityCard.Model] = field(default_factory=list)

        region: list[Region] = field(default_factory=list)

    @dataclass
    class Boundaries:
        @dataclass
        class Boundary:
            name: str
            model: list[RadioactivityCard.Model] = field(default_factory=list)

        boundary: list[Boundary] = field(default_factory=list)

    @dataclass
    class Interfaces:
        @dataclass
        class Interface:
            name: str
            model: list[RadioactivityCard.Model] = field(default_factory=list)

        interface: list[Interface] = field(default_factory=list)

    parameters: ProblemParameters
    initial: Initial
    regions: Regions = field(default_factory=Regions)
    boundaries: Boundaries = field(default_factory=Boundaries)
    interfaces: Interfaces = field(default_factory=Interfaces)

class RadioactivityCardLoader:
    @classmethod
    def load(cls, path: str | Path | None = None) -> RadioactivityCard:
        """读取原始输入结构，空路径生成模板并退出"""
        if path is None or isinstance(path, str) and not path.strip():
            template_path = XmlUtils.write_template(Path(__file__).with_name("radioactivity.xml"), "radioactivity_empty.xml")
            print(f"放射性模板输入卡已生成: {template_path}")
            sys.exit()
        root = XmlUtils.get_root(path, "radioactivity")
        XmlUtils.check_children(root, ('parameters', 'initial', 'regions', 'boundaries', 'interfaces'), context="radioactivity")
        parameters = XmlUtils.read_problem_parameters(root)
        initial = cls.read_initial(XmlUtils.read_child(root, "initial"))
        regions = cls.read_regions(XmlUtils.read_child(root, "regions"))
        boundaries = cls.read_boundaries(XmlUtils.read_child(root, "boundaries", required=False))
        interfaces = cls.read_interfaces(XmlUtils.read_child(root, "interfaces", required=False))
        return RadioactivityCard(parameters, initial, regions, boundaries, interfaces)

    @classmethod
    def read_initial(cls, element: ET.Element) -> RadioactivityCard.Initial:
        concentration = [cls.read_initial_concentration(child) for child in XmlUtils.read_children(element, "concentration", required=True)]
        initial = RadioactivityCard.Initial(concentration=concentration)
        return initial

    @staticmethod
    def read_initial_concentration(element: ET.Element) -> RadioactivityCard.Initial.Concentration:
        region = XmlUtils.read_attribute(element, "region", required=True)
        model_config = XmlUtils.read_model(element, context=f"initial.concentration.{region}")
        concentration = RadioactivityCard.Initial.Concentration(region=region, **model_config)
        return concentration

    @classmethod
    def read_regions(cls, element: ET.Element) -> RadioactivityCard.Regions:
        regions = RadioactivityCard.Regions()
        XmlUtils.check_children(element, ("region",), context="radioactivity.regions")
        for child in XmlUtils.read_children(element, "region", required=True):
            XmlUtils.check_attributes(child, ("name",))
            name = XmlUtils.read_attribute(child, "name", required=True)
            regions.region.append(RadioactivityCard.Regions.Region(name=name, model=cls.read_models(child)))
        return regions

    @classmethod
    def read_boundaries(cls, element: ET.Element | None) -> RadioactivityCard.Boundaries:
        boundaries = RadioactivityCard.Boundaries()
        if element is None:
            return boundaries
        XmlUtils.check_children(element, ("boundary",), context="radioactivity.boundaries")
        for child in XmlUtils.read_children(element, "boundary", required=False):
            XmlUtils.check_attributes(child, ("name",))
            name = XmlUtils.read_attribute(child, "name", required=True)
            boundaries.boundary.append(RadioactivityCard.Boundaries.Boundary(name=name, model=cls.read_models(child)))
        return boundaries

    @classmethod
    def read_interfaces(cls, element: ET.Element | None) -> RadioactivityCard.Interfaces:
        interfaces = RadioactivityCard.Interfaces()
        if element is None:
            return interfaces
        XmlUtils.check_children(element, ("interface",), context="radioactivity.interfaces")
        for child in XmlUtils.read_children(element, "interface", required=False):
            XmlUtils.check_attributes(child, ("name",))
            name = XmlUtils.read_attribute(child, "name", required=True)
            interfaces.interface.append(RadioactivityCard.Interfaces.Interface(name=name, model=cls.read_models(child)))
        return interfaces

    @staticmethod
    def read_models(element: ET.Element) -> list[RadioactivityCard.Model]:
        XmlUtils.check_children(element, ("model",))
        models = []
        for child in XmlUtils.read_children(element, "model", required=False):
            XmlUtils.check_attributes(child, ("name", "model_name"))
            XmlUtils.check_children(child, ("parameter",))
            name = XmlUtils.read_attribute(child, "name", required=True)
            model_name = XmlUtils.read_attribute(child, "model_name", required=True)
            parameters = XmlUtils.read_parameters(child, f"radioactivity.{name}.{model_name}")
            models.append(RadioactivityCard.Model(name=name, model_name=model_name, parameters=parameters))
        return models
