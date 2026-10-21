from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from xml.etree import ElementTree as ET
import math
import sys

from ..xml_utils import XmlUtils
from .template import RadioactivityCardTemplate


@dataclass
class RadioactivityCard:
    """单核素扩散与衰变的初值、边界、界面和源配置"""

    @dataclass
    class Nuclide:
        name: str
        half_life: float  # s

        @property
        def decay_constant(self) -> float:
            """衰变常数, s⁻¹"""
            return math.log(2.0) / self.half_life

    @dataclass
    class Initial:
        @dataclass
        class Concentration:
            region: str
            model: str
            parameters: dict[str, float | list[float]] = field(default_factory=dict)

        concentration: list[Concentration] = field(default_factory=list)

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
        @dataclass
        class Region:
            name: str
            model: str
            parameters: dict[str, float | list[float]] = field(default_factory=dict)

        region: list[Region] = field(default_factory=list)

    nuclide: Nuclide
    initial: Initial
    boundary: list[Boundary] = field(default_factory=list)
    interfaces: Interfaces = field(default_factory=Interfaces)
    source: Source = field(default_factory=Source)


class RadioactivityCardLoader:
    """读取单核素配置, 盘存量按 si_unit 转换为模型所需的原子数单位"""

    @classmethod
    def load(cls, path: str | Path | None = None) -> RadioactivityCard:
        """空路径生成模板并退出, 否则读取放射性输入卡"""
        if path is None or isinstance(path, str) and not path.strip():
            template_path = RadioactivityCardTemplate.write()
            print(f"放射性模板输入卡已生成: {template_path}")
            sys.exit()
        root = XmlUtils.get_root(path, "radioactivity")
        nuclide = cls.read_nuclide(XmlUtils.read_child(root, "nuclide"))
        initial = cls.read_initial(XmlUtils.read_child(root, "initial"), nuclide.decay_constant)
        boundaries = XmlUtils.read_child(root, "boundaries", required=False)
        boundary = []
        if boundaries is not None:
            boundary = [cls.read_boundary(child) for child in XmlUtils.read_children(boundaries, "boundary")]
        interfaces = cls.read_interfaces(XmlUtils.read_child(root, "interfaces", required=False))
        source = cls.read_source(XmlUtils.read_child(root, "source", required=False))
        radioactivity_card = RadioactivityCard(
            nuclide=nuclide, initial=initial, boundary=boundary, interfaces=interfaces, source=source
        )
        return radioactivity_card

    @staticmethod
    def read_nuclide(element: ET.Element) -> RadioactivityCard.Nuclide:
        name = XmlUtils.read_attribute(element, "name", required=True)
        half_life = XmlUtils.read_attribute(XmlUtils.read_child(element, "half_life"), "value", float, required=True, quantity="time")
        # 参数的活度换算需要先得到有效的衰变常数。
        if not math.isfinite(half_life) or half_life <= 0:
            raise ValueError("nuclide.half_life 必须为有限正数")
        nuclide = RadioactivityCard.Nuclide(name=name, half_life=half_life)
        return nuclide

    @classmethod
    def read_initial(cls, element: ET.Element, decay_constant: float) -> RadioactivityCard.Initial:
        concentration = [cls.read_initial_concentration(child, decay_constant)
                         for child in XmlUtils.read_children(element, "concentration", required=True)]
        initial = RadioactivityCard.Initial(concentration=concentration)
        return initial

    @staticmethod
    def read_initial_concentration(element: ET.Element, decay_constant: float) -> RadioactivityCard.Initial.Concentration:
        region = XmlUtils.read_attribute(element, "region", required=True)
        model_config = XmlUtils.read_model(element, decay_constant=decay_constant, context=f"initial.concentration.{region}")
        concentration = RadioactivityCard.Initial.Concentration(region=region, **model_config)
        return concentration

    @staticmethod
    def read_boundary(element: ET.Element) -> RadioactivityCard.Boundary:
        name = XmlUtils.read_attribute(element, "name", required=True)
        boundary = RadioactivityCard.Boundary(name=name, **XmlUtils.read_model(element, context=name))
        return boundary

    @staticmethod
    def read_interfaces(element: ET.Element | None) -> RadioactivityCard.Interfaces:
        interface = []
        if element is not None:
            for child in XmlUtils.read_children(element, "interface"):
                name = XmlUtils.read_attribute(child, "name", required=True)
                interface.append(RadioactivityCard.Interfaces.Interface(name=name, **XmlUtils.read_model(child, context=name)))
        interfaces = RadioactivityCard.Interfaces(interface=interface)
        return interfaces

    @staticmethod
    def read_source(element: ET.Element | None) -> RadioactivityCard.Source:
        region = []
        if element is not None:
            for child in XmlUtils.read_children(element, "region"):
                name = XmlUtils.read_attribute(child, "name", required=True)
                region.append(RadioactivityCard.Source.Region(name=name, **XmlUtils.read_model(child, context=name)))
        source = RadioactivityCard.Source(region=region)
        return source
