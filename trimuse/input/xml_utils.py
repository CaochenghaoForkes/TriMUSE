"""XML 通用读取工具；具体输入卡规则由各自的 Examine 类检查

属性读取默认允许缺省并返回默认值，标记为必填时缺失或空值报错
context 可传入文件名和节点路径，使错误信息能够定位到输入卡
物理量只提取 value、unit 单位解释与换算由单位模块完成
"""

from __future__ import annotations
import math
import re
from pathlib import Path
from typing import Iterable
from xml.etree import ElementTree as ET

class XmlUtils:
    """无实例状态的 XML 工具集合，直接通过类名调用"""

    @staticmethod
    def _location(element: ET.Element, context: str | None = None) -> str:
        """统一构造错误位置"""
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
    def read_xml(path: str | Path, root_tag: str | None = None) -> ET.Element:
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
    def read_string(
        element: ET.Element,
        name: str,
        *,
        required: bool = False,
        default: str | None = None,
        context: str | None = None,
    ) -> str | None:
        """读取属性并去除首尾空白；显式空值报错，不用默认值掩盖"""
        value = element.get(name)
        location = XmlUtils._location(element, context)
        if value is None:
            if required:
                raise ValueError(f"{location}: 缺少必填属性 '{name}'")
            return default
        value = value.strip()
        if not value:
            raise ValueError(f"{location}: 属性 '{name}' 不能为空")
        return value

    @staticmethod
    def read_int(
        element: ET.Element,
        name: str,
        *,
        required: bool = False,
        default: int | None = None,
        context: str | None = None,
    ) -> int | None:
        """读取十进制整数；不接受小数、科学计数法或布尔字符串"""
        value = XmlUtils.read_string(element, name, required=required, context=context)
        if value is None:
            return default
        if re.fullmatch(r"[+-]?[0-9]+", value) is None:
            raise ValueError(
                f"{XmlUtils._location(element, context)}: "
                f"属性 '{name}' 必须为整数，实际为 {value!r}"
            )
        return int(value)

    @staticmethod
    def read_float(
        element: ET.Element,
        name: str,
        *,
        required: bool = False,
        default: float | None = None,
        context: str | None = None,
    ) -> float | None:
        """读取有限浮点数，支持科学计数法，拒绝 NaN 和无穷大"""
        value = XmlUtils.read_string(element, name, required=required, context=context)
        if value is None:
            return default
        pattern = r"[+-]?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?"
        location = XmlUtils._location(element, context)
        if re.fullmatch(pattern, value) is None:
            raise ValueError(f"{location}: 属性 '{name}' 必须为有限数值，实际为 {value!r}")
        number = float(value)
        if not math.isfinite(number):
            raise ValueError(f"{location}: 属性 '{name}' 超出有限数值范围: {value!r}")
        return number

    @staticmethod
    def read_bool(
        element: ET.Element,
        name: str,
        *,
        required: bool = False,
        default: bool | None = None,
        context: str | None = None,
    ) -> bool | None:
        """接受 true、false (忽略大小写) 以及 1、0"""
        value = XmlUtils.read_string(element, name, required=required, context=context)
        if value is None:
            return default
        normalized = value.lower()
        if normalized in ("true", "1"):
            return True
        if normalized in ("false", "0"):
            return False
        raise ValueError(
            f"{XmlUtils._location(element, context)}: "
            f"属性 '{name}' 必须为 true、false、1 或 0, 实际为 {value!r}"
        )

    @staticmethod
    def read_text(
        element: ET.Element,
        *,
        required: bool = False,
        default: str | None = None,
        context: str | None = None,
    ) -> str | None:
        """读取纯文本元素正文；拒绝嵌套元素，避免漏读正文"""
        location = XmlUtils._location(element, context)
        if any(isinstance(child.tag, str) for child in element):
            raise ValueError(f"{location}: 纯文本节点不能包含子元素")
        value = (element.text or "").strip()
        if not value:
            if required:
                raise ValueError(f"{location}: 正文不能为空")
            return default
        return value

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
    def read_quantity(
        element: ET.Element, *, context: str | None = None
    ) -> tuple[float, str]:
        """读取物理量的 value 和 unit 属性，不解释单位表达式"""
        XmlUtils.check_attributes(element, ("value", "unit"), context=context)
        XmlUtils.check_children(element, (), context=context)
        if (element.text or "").strip():
            raise ValueError(
                f"{XmlUtils._location(element, context)}: "
                "物理量应使用 value、unit 属性，不能包含正文"
            )
        value = XmlUtils.read_float(element, "value", required=True, context=context)
        unit = XmlUtils.read_string(element, "unit", required=True, context=context)
        # 必填检查已保证两者存在；断言同时帮助静态类型检查
        assert value is not None and unit is not None
        return value, unit

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
        """以声明路径的 XML 所在目录为基准，返回绝对路径

        不要求目标已存在，因为输出文件路径也使用此方法
        """
        if isinstance(path, str) and not path.strip():
            raise ValueError("文件路径不能为空")
        resolved = Path(path).expanduser()
        if not resolved.is_absolute():
            resolved = Path(base_directory).expanduser().resolve() / resolved
        return resolved.resolve()
