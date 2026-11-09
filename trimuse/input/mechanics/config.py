from dataclasses import dataclass
from pathlib import Path
import sys

from ..xml_utils import XmlUtils

@dataclass
class MechanicsCard:
    """力学初始化配置，当前读取全局热膨胀参考温度"""

    thermal_expansion_reference_temperature: float

class MechanicsCardLoader:
    @classmethod
    def load(cls, path: str | Path | None = None) -> MechanicsCard:
        if path is None or isinstance(path, str) and not path.strip():
            template_path = XmlUtils.write_template(Path(__file__).with_name("mechanics.xml"), "mechanics_empty.xml")
            print(f"力学模板输入卡已生成: {template_path}")
            sys.exit()
        root = XmlUtils.get_root(path, "mechanics")
        reference = XmlUtils.read_child(root, "thermal_expansion_reference_temperature")
        temperature = XmlUtils.read_attribute(reference, "value", float, required=True, quantity="temperature")
        return MechanicsCard(thermal_expansion_reference_temperature=temperature)
