from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from xml.etree import ElementTree as ET
import sys

from ..xml_utils import XmlUtils
from .template import PostprocessCardTemplate

@dataclass
class PostprocessCard:
    """后处理位置与待提取属性, 坐标为 m, 角度为 rad"""

    @dataclass
    class Center:
        x: float
        y: float | None = None
        z: float | None = None

    @dataclass
    class Direction:
        """二维 theta 从 +y 起算, 三维 theta 从 +z 起算, phi 绕 z 轴"""

        theta: float
        phi: float | None = None

    @dataclass
    class Point:
        name: str
        center: PostprocessCard.Center

    @dataclass
    class Line:
        """从颗粒中心沿指定方向到外表面的射线; 1D 为整个径向范围"""

        name: str
        sample_count: int = 100
        direction: PostprocessCard.Direction | None = None

    @dataclass
    class Section:
        """2D 为整个计算截面, 3D 由中心和法线指定平面"""

        name: str
        center: PostprocessCard.Center | None = None
        normal: PostprocessCard.Direction | None = None

    properties: list[str]
    points: list[Point] = field(default_factory=list)
    lines: list[Line] = field(default_factory=list)
    sections: list[Section] = field(default_factory=list)

class PostprocessCardLoader:
    @classmethod
    def load(cls, path: str | Path | None = None) -> PostprocessCard:
        """空路径生成模板并退出, 否则读取后处理卡"""
        if path is None or isinstance(path, str) and not path.strip():
            template_path = PostprocessCardTemplate.write()
            print(f"后处理模板输入卡已生成: {template_path}")
            sys.exit()
        root = XmlUtils.get_root(path, "postprocess")
        points_element = XmlUtils.read_child(root, "points", required=False)
        lines_element = XmlUtils.read_child(root, "lines", required=False)
        sections_element = XmlUtils.read_child(root, "sections", required=False)
        points = [cls.read_point(child) for child in XmlUtils.read_children(points_element, "point")] if points_element is not None else []
        lines = [cls.read_line(child) for child in XmlUtils.read_children(lines_element, "line")] if lines_element is not None else []
        sections = [cls.read_section(child) for child in XmlUtils.read_children(sections_element, "section")] if sections_element is not None else []
        properties_element = XmlUtils.read_child(root, "properties")
        properties = XmlUtils.read_attribute(properties_element, "names", list[str], required=True)
        postprocess_card = PostprocessCard(properties=properties, points=points, lines=lines, sections=sections)
        return postprocess_card

    @staticmethod
    def read_center(element: ET.Element | None) -> PostprocessCard.Center | None:
        if element is None:
            return None
        x = XmlUtils.read_attribute(XmlUtils.read_child(element, "x"), "value", float, required=True, quantity="length")
        y = XmlUtils.read_attribute(XmlUtils.read_child(element, "y", required=False), "value", float, quantity="length")
        z = XmlUtils.read_attribute(XmlUtils.read_child(element, "z", required=False), "value", float, quantity="length")
        center = PostprocessCard.Center(x=x, y=y, z=z)
        return center

    @staticmethod
    def read_direction(element: ET.Element | None) -> PostprocessCard.Direction | None:
        if element is None:
            return None
        theta = XmlUtils.read_attribute(XmlUtils.read_child(element, "theta"), "value", float, required=True, quantity="angle")
        phi_element = XmlUtils.read_child(element, "phi", required=False)
        phi = XmlUtils.read_attribute(phi_element, "value", float, required=True, quantity="angle") if phi_element is not None else None
        direction = PostprocessCard.Direction(theta=theta, phi=phi)
        return direction

    @classmethod
    def read_point(cls, element: ET.Element) -> PostprocessCard.Point:
        name = XmlUtils.read_attribute(element, "name", required=True)
        center = cls.read_center(XmlUtils.read_child(element, "center"))
        point = PostprocessCard.Point(name=name, center=center)
        return point

    @classmethod
    def read_line(cls, element: ET.Element) -> PostprocessCard.Line:
        name = XmlUtils.read_attribute(element, "name", required=True)
        sample_count = XmlUtils.read_attribute(element, "sample_count", int, default=100)
        direction = cls.read_direction(XmlUtils.read_child(element, "direction", required=False))
        line = PostprocessCard.Line(name=name, sample_count=sample_count, direction=direction)
        return line

    @classmethod
    def read_section(cls, element: ET.Element) -> PostprocessCard.Section:
        name = XmlUtils.read_attribute(element, "name", required=True)
        center = cls.read_center(XmlUtils.read_child(element, "center", required=False))
        normal = cls.read_direction(XmlUtils.read_child(element, "normal", required=False))
        section = PostprocessCard.Section(name=name, center=center, normal=normal)
        return section
