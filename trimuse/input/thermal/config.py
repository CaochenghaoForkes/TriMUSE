from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from xml.etree import ElementTree as ET
import sys

from ..xml_utils import XmlUtils
from .template import ThermalCardTemplate

@dataclass
class ThermalCard:
    """热问题的初始温度, 边界, 界面和热源配置"""

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
    class Boundary:
        """按外边界名称配置热边界模型"""

        name: str
        model: str
        parameters: dict[str, float | list[float]] = field(default_factory=dict)

    @dataclass
    class Interfaces:
        """内部材料界面的传热关系"""

        @dataclass
        class Interface:
            """理想接触或后续的界面传热模型"""

            name: str
            model: str
            parameters: dict[str, float | list[float]] = field(default_factory=dict)

        interface: list[Interface] = field(default_factory=list)

    @dataclass
    class Source:
        """各区域的体积热源或总功率"""

        @dataclass
        class Region:
            """选择向热方程提供 W/m³ 的区域热源模型"""

            name: str
            model: str
            parameters: dict[str, float | list[float]] = field(default_factory=dict)

        region: list[Region] = field(default_factory=list)

    initial: Initial
    boundary: list[Boundary] = field(default_factory=list)
    interfaces: Interfaces = field(default_factory=Interfaces)
    source: Source = field(default_factory=Source)

class ThermalCardLoader:
    """读取热配置, 模型参数统一调用 XmlUtils.read_model"""

    @classmethod
    def load(cls, path: str | Path | None = None) -> ThermalCard:
        """空路径在当前工作目录生成模板并退出, 否则读取热输入卡"""
        if path is None or isinstance(path, str) and not path.strip():
            template_path = ThermalCardTemplate.write()
            print(f"热模板输入卡已生成: {template_path}")
            sys.exit()
        root = XmlUtils.get_root(path, "thermal")
        initial = cls.read_initial(XmlUtils.read_child(root, "initial"))
        boundaries = XmlUtils.read_child(root, "boundaries", required=False)
        boundary = [cls.read_boundary(element) for element in XmlUtils.read_children(boundaries, "boundary")] if boundaries is not None else []
        interfaces = cls.read_interfaces(XmlUtils.read_child(root, "interfaces", required=False))
        source = cls.read_source(XmlUtils.read_child(root, "source", required=False))
        thermal_card = ThermalCard(initial=initial, boundary=boundary, interfaces=interfaces, source=source)
        return thermal_card

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

    @staticmethod
    def read_boundary(element: ET.Element) -> ThermalCard.Boundary:
        """直接读取边界模型及其参数"""
        name = XmlUtils.read_attribute(element, "name", required=True)
        model_config = XmlUtils.read_model(element, context=name)
        boundary = ThermalCard.Boundary(name=name, **model_config)
        return boundary

    @classmethod
    def read_interfaces(cls, element: ET.Element | None) -> ThermalCard.Interfaces:
        """未指定界面关系时保留空列表, 采用默认理想接触"""
        interface = []
        if element is not None:
            interface = [cls.read_interface(child) for child in XmlUtils.read_children(element, "interface")]

        interfaces = ThermalCard.Interfaces(interface=interface)
        return interfaces

    @staticmethod
    def read_interface(element: ET.Element) -> ThermalCard.Interfaces.Interface:
        """读取界面名称和模型"""
        name = XmlUtils.read_attribute(element, "name", required=True)
        model_config = XmlUtils.read_model(element, context=name)
        interface = ThermalCard.Interfaces.Interface(name=name, **model_config)
        return interface

    @classmethod
    def read_source(cls, element: ET.Element | None) -> ThermalCard.Source:
        """未指定区域热源时保留空列表, 对应零热源"""
        region = []
        if element is not None:
            region = [cls.read_source_region(child) for child in XmlUtils.read_children(element, "region")]
        source = ThermalCard.Source(region=region)
        return source

    @staticmethod
    def read_source_region(element: ET.Element) -> ThermalCard.Source.Region:
        """读取区域热源模型, 总功率转换由后续热模块初始化负责"""
        name = XmlUtils.read_attribute(element, "name", required=True)
        model_config = XmlUtils.read_model(element, context=name)
        region = ThermalCard.Source.Region(name=name, **model_config)
        return region
