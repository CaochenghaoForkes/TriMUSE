from __future__ import annotations
import re
from pathlib import Path
from types import GenericAlias
from typing import Iterable, get_args, get_origin
from xml.etree import ElementTree as ET

from .units import Units

class XmlUtils:
    """无实例状态的 XML 工具集合，直接通过类名调用"""

    @staticmethod
    def _location(element: ET.Element | None, context: str | None = None) -> str:
        """统一构造错误位置"""
        if element is None:
            return context or "XML"
        return f"{context}: <{element.tag}>" if context else f"<{element.tag}>"

    @staticmethod
    def write_template(content: str, path: str | Path) -> Path:
        """写出待填写的 XML 模板, 同名文件已存在时拒绝覆盖"""
        ET.fromstring(content)
        output_path = Path(path).expanduser().absolute()
        output_path.parent.mkdir(parents=True, exist_ok=True)
        with output_path.open("x", encoding="utf-8") as stream:
            stream.write(content)
        return output_path

    @staticmethod
    def get_root(path: str | Path, root_tag: str | None = None) -> ET.Element:
        """读取 XML 并可选核对根标签；不创建目录或修改文件"""
        card_path = Path(path).expanduser().resolve()
        if not card_path.is_file():
            raise FileNotFoundError(f"XML 输入文件不存在或不是普通文件: {card_path}")
        try:
            root = ET.parse(card_path).getroot()
        except ET.ParseError as exc:
            raise ValueError(f"{card_path}: XML 格式错误: {exc}") from exc
        if root_tag is not None and root.tag != root_tag:
            raise ValueError(f"{card_path}: 根节点应为 <{root_tag}>，实际为 <{root.tag}>")
        return root

    @staticmethod
    def read_attribute(
        element: ET.Element | None,
        name: str,
        value_type: type | GenericAlias = str,
        *,
        required: bool = False,
        default: str | int | float | bool | list | None = None,
        allow_empty: bool = False,
        quantity: str | None = None,
        context: str | None = None,
    ) -> str | int | float | bool | list | None:
        value = element.get(name) if element is not None else None
        location = XmlUtils._location(element, context)
        if value is None:
            if required:
                raise ValueError(f"{location}: 缺少必填属性 '{name}'")
            return default
        value = value.strip()
        if not value:
            if allow_empty:
                return default
            raise ValueError(f"{location}: 属性 '{name}' 不能为空")
        is_array = get_origin(value_type) is list
        item_type = get_args(value_type)[0] if is_array else value_type
        items = value.split() if is_array else [value]
        result = []
        for item in items:
            if item_type is bool:
                result.append({"true": True, "false": False}[item.lower()])
            elif item_type is int:
                result.append(int(item))
            else:
                result.append(item_type(item))
        value = result if is_array else result[0]
        if quantity is not None:
            unit = XmlUtils.read_attribute(element, "unit", str, required=True, context=context)
            value = Units.convert(value, unit, Units.SI_UNITS[quantity], context=context)
        return value

    @staticmethod
    def read_model(element: ET.Element, *, context: str | None = None) -> dict[str, str | dict[str, float | list[float]]]:
        """读取模型参数并换算单位, 单个数值存为标量, 空白分隔的多个数值存为列表"""
        model = XmlUtils.read_attribute(element, "model", required=True, context=context)
        location = XmlUtils._location(element, context)
        parameters = {}
        for parameter_element in XmlUtils.read_children(element, "parameter"):
            name = XmlUtils.read_attribute(parameter_element, "name", required=True, context=context)
            values = XmlUtils.read_attribute(parameter_element, "value", list[float], required=True, context=context)
            value = values[0] if len(values) == 1 else values
            unit = XmlUtils.read_attribute(parameter_element, "unit", required=True, context=context)
            si_unit = XmlUtils.read_attribute(parameter_element, "si_unit", required=True, context=context)
            if name in parameters:
                raise ValueError(f"{location}: 参数名重复: {name}")
            parameters[name] = Units.convert(value, unit, si_unit, context=context)
        model_config = {"model": model, "parameters": parameters}
        return model_config

    @staticmethod
    def read_child(
        element: ET.Element,
        name: str,
        *,
        required: bool = True,
        context: str | None = None,
    ) -> ET.Element | None:
        """按标签读取唯一的直接子元素，不进行递归或 XPath 查询"""
        children = XmlUtils.read_children(element, name)
        location = XmlUtils._location(element, context)
        if len(children) > 1:
            raise ValueError(f"{location}: 子节点 <{name}> 只能出现一次")
        if not children:
            if required:
                raise ValueError(f"{location}: 缺少必需子节点 <{name}>")
            return None
        return children[0]

    @staticmethod
    def read_children(
        element: ET.Element,
        name: str,
        *,
        required: bool = False,
        context: str | None = None,
    ) -> list[ET.Element]:
        """按文件书写顺序读取同名直接子元素；标记为必需时要求至少一个"""
        children = [child for child in element if child.tag == name]
        if required and not children:
            raise ValueError(
                f"{XmlUtils._location(element, context)}: "
                f"至少需要一个子节点 <{name}>"
            )
        return children

    @staticmethod
    def check_attributes(
        element: ET.Element,
        allowed: Iterable[str],
        *,
        context: str | None = None,
    ) -> None:
        """拒绝未知属性；必填检查由属性读取方法承担"""
        unknown = set(element.attrib) - set(allowed)
        if unknown:
            raise ValueError(
                f"{XmlUtils._location(element, context)}: "
                f"未知属性: {', '.join(sorted(unknown))}"
            )

    @staticmethod
    def check_children(
        element: ET.Element,
        allowed: Iterable[str],
        *,
        context: str | None = None,
    ) -> None:
        """拒绝未知直接子元素；忽略 XML 注释，数量由读取方法检查"""
        allowed_names = set(allowed)
        unknown = {
            child.tag for child in element
            if isinstance(child.tag, str) and child.tag not in allowed_names
        }
        if unknown:
            raise ValueError(
                f"{XmlUtils._location(element, context)}: "
                f"未知子节点: {', '.join(sorted(unknown))}"
            )

    @staticmethod
    def resolve_path(path: str | Path, base_directory: str | Path) -> Path:
        """以声明路径的 XML 所在目录为基准，返回绝对路径不要求目标已存在，因为输出文件路径也使用此方法"""
        if isinstance(path, str) and not path.strip():
            raise ValueError("文件路径不能为空")
        resolved = Path(path).expanduser()
        if not resolved.is_absolute():
            resolved = Path(base_directory).expanduser().resolve() / resolved
        return resolved.resolve()
