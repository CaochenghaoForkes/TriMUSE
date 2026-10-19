from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from xml.etree import ElementTree as ET
import sys

from ..xml_utils import XmlUtils
from .template import MaterialCardTemplate

@dataclass
class MaterialCard:
    """材料定义和区域分配, 参数数值使用模型约定的内部单位"""

    @dataclass
    class Property:
        """物性名称, 模型及对应的构造参数字典"""

        property_name: str  # 例如 thermal.conductivity
        model: str
        parameters: dict[str, float | list[float]] = field(default_factory=dict)

    @dataclass
    class Mat:
        """独立材料定义, 同一材料可分配给多个区域"""

        name: str
        properties: list[MaterialCard.Property] = field(default_factory=list)

    @dataclass
    class Assign:
        """材料与几何区域的关联"""

        @dataclass
        class Region:
            """区域名称引用几何卡, mat 引用本卡材料名称"""

            name: str
            mat: str

        region: list[Region] = field(default_factory=list)

    mat: list[Mat] = field(default_factory=list)
    assign: Assign = field(default_factory=Assign)

class MaterialCardLoader:
    """统一读取各物性模型, 不在输入层硬编码模型参数"""

    @classmethod
    def load(cls, path: str | Path | None = None) -> MaterialCard:
        """空路径在当前工作目录生成模板并退出, 否则读取材料卡"""
        if path is None or isinstance(path, str) and not path.strip():
            template_path = MaterialCardTemplate.write()
            print(f"材料模板输入卡已生成: {template_path}")
            sys.exit()
        root = XmlUtils.get_root(path, "material")
        mat = [cls.read_mat(element) for element in XmlUtils.read_children(root, "mat", required=True)]
        assign = cls.read_assign(XmlUtils.read_child(root, "assign"))
        material_card = MaterialCard(mat=mat, assign=assign)
        return material_card

    @classmethod
    def read_mat(cls, element: ET.Element) -> MaterialCard.Mat:
        """按物理分类读取物性, 包括 thermal.conductivity 和 thermal.specific_heat"""
        name = XmlUtils.read_attribute(element, "name", required=True)
        properties = []
        for physics_element in element:
            for property_element in physics_element:
                property_config = cls.read_property(property_element, physics_element.tag)
                properties.append(property_config)
        mat = MaterialCard.Mat(name=name, properties=properties)
        return mat

    @staticmethod
    def read_property(element: ET.Element, physics_name: str) -> MaterialCard.Property:
        """复用通用模型读取方法, 无构造参数时使用空字典"""
        property_name = f"{physics_name}.{element.tag}"
        model_config = XmlUtils.read_model(element, context=property_name)
        model = model_config["model"]
        parameters = model_config["parameters"]
        property_config = MaterialCard.Property(
            property_name=property_name,
            model=model,
            parameters=parameters
        )
        return property_config

    @classmethod
    def read_assign(cls, element: ET.Element) -> MaterialCard.Assign:
        """读取区域分配关系, 保留输入顺序"""
        region = [cls.read_region(child) for child in XmlUtils.read_children(element, "region", required=True)]
        assign = MaterialCard.Assign(region=region)
        return assign

    @staticmethod
    def read_region(element: ET.Element) -> MaterialCard.Assign.Region:
        """读取区域名称及材料名称"""
        name = XmlUtils.read_attribute(element, "name", required=True)
        mat = XmlUtils.read_attribute(element, "mat", required=True)
        region = MaterialCard.Assign.Region(name=name, mat=mat)
        return region
