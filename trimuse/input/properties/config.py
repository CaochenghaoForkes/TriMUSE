from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from xml.etree import ElementTree as ET
import sys

from ..xml_utils import XmlUtils
from .template import PropertiesCardTemplate

@dataclass
class PropertiesCard:
    """按全局、材料区域或外表面分区配置外部量和派生量"""

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
    class Property:
        name: str
        region: list[PropertiesCard.Region] = field(default_factory=list)
        zone: list[PropertiesCard.Zone] = field(default_factory=list)
        global_: PropertiesCard.Global | None = None

    external_property: list[Property] = field(default_factory=list)
    derived_property: list[Property] = field(default_factory=list)

class PropertiesCardLoader:
    # 仅这些属性中的 mol、活度单位按核素数量换算, 不影响其他物理量
    RADIOACTIVITY_PROPERTY_NAMES = {
        "nuclide_birth_rate",
        "volumetric_nuclide_birth_rate",
        "environment_nuclide_concentration",
    }

    @classmethod
    def load(cls, path: str | Path | None = None, *, decay_constant: float | None = None) -> PropertiesCard:
        """空路径生成模板, 否则读取外部量和派生量配置"""
        if path is None or isinstance(path, str) and not path.strip():
            template_path = PropertiesCardTemplate.write()
            print(f"属性模板输入卡已生成: {template_path}")
            sys.exit()
        root = XmlUtils.get_root(path, "properties")
        external_property = [cls.read_property(element, decay_constant=decay_constant) for element in XmlUtils.read_children(root, "external_property")]
        derived_property = [cls.read_property(element, decay_constant=decay_constant) for element in XmlUtils.read_children(root, "derived_property")]
        return PropertiesCard(external_property=external_property, derived_property=derived_property)

    @classmethod
    def read_property(cls, element: ET.Element, *, decay_constant: float | None = None) -> PropertiesCard.Property:
        name = XmlUtils.read_attribute(element, "name", required=True)
        if name not in cls.RADIOACTIVITY_PROPERTY_NAMES:
            decay_constant = None
        region = [cls.read_region(child, decay_constant=decay_constant) for child in XmlUtils.read_children(element, "region")]
        zone = [cls.read_zone(child, decay_constant=decay_constant) for child in XmlUtils.read_children(element, "zone")]
        global_element = XmlUtils.read_child(element, "global", required=False)
        global_ = cls.read_global(global_element, decay_constant=decay_constant) if global_element is not None else None
        return PropertiesCard.Property(name=name, region=region, zone=zone, global_=global_)

    @staticmethod
    def read_region(element: ET.Element, *, decay_constant: float | None = None) -> PropertiesCard.Region:
        name = XmlUtils.read_attribute(element, "name", required=True)
        model_config = XmlUtils.read_model(element, decay_constant=decay_constant, context=name)
        return PropertiesCard.Region(name=name, **model_config)

    @staticmethod
    def read_global(element: ET.Element, *, decay_constant: float | None = None) -> PropertiesCard.Global:
        model_config = XmlUtils.read_model(element, decay_constant=decay_constant, context="global")
        return PropertiesCard.Global(**model_config)

    @staticmethod
    def read_zone(element: ET.Element, *, decay_constant: float | None = None) -> PropertiesCard.Zone:
        """统一读取 constant 的标量或 time_series 的 times/values 参数"""
        name = XmlUtils.read_attribute(element, "name", required=True)
        model_config = XmlUtils.read_model(element, decay_constant=decay_constant, context=name)
        return PropertiesCard.Zone(name=name, **model_config)
