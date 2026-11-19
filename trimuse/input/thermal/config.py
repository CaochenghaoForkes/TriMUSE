from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from xml.etree import ElementTree as ET
import sys

from ..xml_utils import ProblemParameters, XmlUtils

@dataclass
class ThermalCard:
    """按 XML 层级保存热问题配置，位置名称保持原始字符串"""

    @dataclass
    class Initial:
        """分区域初始温度"""

        @dataclass
        class Temperature:
            """区域名称, 初始温度模型及换算后的参数"""

            region: str
            model: str
            parameters: dict[str, float | list[float]] = field(default_factory=dict)

        temperature: list[Temperature] = field(default_factory=list)

    @dataclass
    class Model:
        name: str
        model_name: str

    @dataclass
    class Regions:
        @dataclass
        class Region:
            name: str
            model: list[ThermalCard.Model] = field(default_factory=list)

        region: list[Region] = field(default_factory=list)

    @dataclass
    class Boundaries:
        @dataclass
        class Boundary:
            name: str
            model: list[ThermalCard.Model] = field(default_factory=list)

        boundary: list[Boundary] = field(default_factory=list)

    @dataclass
    class Interfaces:
        @dataclass
        class Interface:
            name: str
            model: list[ThermalCard.Model] = field(default_factory=list)

        interface: list[Interface] = field(default_factory=list)

    parameters: ProblemParameters
    initial: Initial
    regions: Regions = field(default_factory=Regions)
    boundaries: Boundaries = field(default_factory=Boundaries)
    interfaces: Interfaces = field(default_factory=Interfaces)

class ThermalCardLoader:
    @classmethod
    def load(cls, path: str | Path | None = None) -> ThermalCard:
        """读取原始输入结构，空路径生成模板并退出"""
        if path is None or isinstance(path, str) and not path.strip():
            template_path = XmlUtils.write_template(Path(__file__).with_name("thermal.xml"), "thermal_empty.xml")
            print(f"热模板输入卡已生成: {template_path}")
            sys.exit()
        root = XmlUtils.get_root(path, "thermal")
        XmlUtils.check_children(root, ('parameters', 'initial', 'regions', 'boundaries', 'interfaces'), context="thermal")
        parameters = XmlUtils.read_problem_parameters(root)
        initial = cls.read_initial(XmlUtils.read_child(root, "initial"))
        regions = cls.read_regions(XmlUtils.read_child(root, "regions"))
        boundaries = cls.read_boundaries(XmlUtils.read_child(root, "boundaries", required=False))
        interfaces = cls.read_interfaces(XmlUtils.read_child(root, "interfaces", required=False))
        return ThermalCard(parameters, initial, regions, boundaries, interfaces)

    @classmethod
    def read_initial(cls, element: ET.Element) -> ThermalCard.Initial:
        """按输入顺序读取各区域的初始温度"""
        temperature = [cls.read_initial_temperature(child) for child in XmlUtils.read_children(element, "temperature", required=True)]
        initial = ThermalCard.Initial(temperature=temperature)
        return initial

    @staticmethod
    def read_initial_temperature(element: ET.Element) -> ThermalCard.Initial.Temperature:
        """读取区域名称和初始温度模型"""
        region = XmlUtils.read_attribute(element, "region", required=True)
        model_config = XmlUtils.read_model(element, context=f"initial.temperature.{region}")
        temperature = ThermalCard.Initial.Temperature(region=region, **model_config)
        return temperature

    @classmethod
    def read_regions(cls, element: ET.Element) -> ThermalCard.Regions:
        regions = ThermalCard.Regions()
        XmlUtils.check_children(element, ("region",), context="thermal.regions")
        for child in XmlUtils.read_children(element, "region", required=True):
            XmlUtils.check_attributes(child, ("name",))
            name = XmlUtils.read_attribute(child, "name", required=True)
            regions.region.append(ThermalCard.Regions.Region(name=name, model=cls.read_models(child)))
        return regions

    @classmethod
    def read_boundaries(cls, element: ET.Element | None) -> ThermalCard.Boundaries:
        boundaries = ThermalCard.Boundaries()
        if element is None:
            return boundaries
        XmlUtils.check_children(element, ("boundary",), context="thermal.boundaries")
        for child in XmlUtils.read_children(element, "boundary", required=False):
            XmlUtils.check_attributes(child, ("name",))
            name = XmlUtils.read_attribute(child, "name", required=True)
            boundaries.boundary.append(ThermalCard.Boundaries.Boundary(name=name, model=cls.read_models(child)))
        return boundaries

    @classmethod
    def read_interfaces(cls, element: ET.Element | None) -> ThermalCard.Interfaces:
        interfaces = ThermalCard.Interfaces()
        if element is None:
            return interfaces
        XmlUtils.check_children(element, ("interface",), context="thermal.interfaces")
        for child in XmlUtils.read_children(element, "interface", required=False):
            XmlUtils.check_attributes(child, ("name",))
            name = XmlUtils.read_attribute(child, "name", required=True)
            interfaces.interface.append(ThermalCard.Interfaces.Interface(name=name, model=cls.read_models(child)))
        return interfaces

    @staticmethod
    def read_models(element: ET.Element) -> list[ThermalCard.Model]:
        XmlUtils.check_children(element, ("model",))
        models = []
        for child in XmlUtils.read_children(element, "model", required=False):
            XmlUtils.check_attributes(child, ("name", "model_name"))
            XmlUtils.check_children(child, ())
            name = XmlUtils.read_attribute(child, "name", required=True)
            model_name = XmlUtils.read_attribute(child, "model_name", required=True)
            models.append(ThermalCard.Model(name=name, model_name=model_name))
        return models
