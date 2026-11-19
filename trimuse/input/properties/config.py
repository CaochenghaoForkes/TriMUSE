from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from xml.etree import ElementTree as ET
import sys

from ..xml_utils import XmlUtils

@dataclass
class PropertiesCard:
    """按全局、材料区域、外表面分区或界面配置属性"""

    @dataclass
    class Region:
        name: str
        model: str
        parameters: dict[str, float | list[float]] = field(default_factory=dict)

    @dataclass
    class Global:
        model: str
        parameters: dict[str, float | list[float]] = field(default_factory=dict)

    @dataclass
    class Zone:
        """外表面分区模型; 温度使用 K, heat_flux 使用 W/m² (外流为正)"""

        name: str
        model: str
        parameters: dict[str, float | list[float]] = field(default_factory=dict)

    @dataclass
    class Interface:
        name: str
        model: str
        parameters: dict[str, float | list[float]] = field(default_factory=dict)

    @dataclass
    class Property:
        name: str  # 完整属性名, 如 external.power; XML 仅填写 power
        region: list[PropertiesCard.Region] = field(default_factory=list)
        zone: list[PropertiesCard.Zone] = field(default_factory=list)
        global_: PropertiesCard.Global | None = None
        interface: list[PropertiesCard.Interface] = field(default_factory=list)

    external_property: list[Property] = field(default_factory=list)
    derived_property: list[Property] = field(default_factory=list)
    assembly_property: list[Property] = field(default_factory=list)

class PropertiesCardLoader:
    @classmethod
    def load(cls, path: str | Path | None = None) -> PropertiesCard:
        """空路径生成模板, 否则读取外部量和派生量配置"""
        if path is None or isinstance(path, str) and not path.strip():
            template_path = XmlUtils.write_template(Path(__file__).with_name("properties.xml"), "properties_empty.xml")
            print(f"属性模板输入卡已生成: {template_path}")
            sys.exit()
        root = XmlUtils.get_root(path, "properties")
        external_property = [cls.read_property(element) for element in XmlUtils.read_children(root, "external_property")]
        derived_property = [cls.read_property(element) for element in XmlUtils.read_children(root, "derived_property")]
        assembly_property = [cls.read_property(element) for element in XmlUtils.read_children(root, "assembly_property")]
        return PropertiesCard(external_property=external_property, derived_property=derived_property, assembly_property=assembly_property)

    @classmethod
    def read_property(cls, element: ET.Element) -> PropertiesCard.Property:
        name = f'{element.tag.removesuffix("_property")}.{XmlUtils.read_attribute(element, "name", required=True)}'
        region = [cls.read_region(child) for child in XmlUtils.read_children(element, "region")]
        zone = [cls.read_zone(child) for child in XmlUtils.read_children(element, "zone")]
        interface = [cls.read_interface(child) for child in XmlUtils.read_children(element, "interface")]
        global_element = XmlUtils.read_child(element, "global", required=False)
        global_ = cls.read_global(global_element) if global_element is not None else None
        return PropertiesCard.Property(name=name, region=region, zone=zone, global_=global_, interface=interface)

    @staticmethod
    def read_region(element: ET.Element) -> PropertiesCard.Region:
        name = XmlUtils.read_attribute(element, "name", required=True)
        model_config = XmlUtils.read_model(element, context=name)
        return PropertiesCard.Region(name=name, **model_config)

    @staticmethod
    def read_global(element: ET.Element) -> PropertiesCard.Global:
        model_config = XmlUtils.read_model(element, context="global")
        return PropertiesCard.Global(**model_config)

    @staticmethod
    def read_zone(element: ET.Element) -> PropertiesCard.Zone:
        """读取常数或时间表模型参数，数值可以是标量或向量"""
        name = XmlUtils.read_attribute(element, "name", required=True)
        model_config = XmlUtils.read_model(element, context=name)
        return PropertiesCard.Zone(name=name, **model_config)

    @staticmethod
    def read_interface(element: ET.Element) -> PropertiesCard.Interface:
        name = XmlUtils.read_attribute(element, "name", required=True)
        model_config = XmlUtils.read_model(element, context=name)
        if "." not in model_config["model"]:
            model_config["model"] = f'common.{model_config["model"]}'
        return PropertiesCard.Interface(name=name, **model_config)
