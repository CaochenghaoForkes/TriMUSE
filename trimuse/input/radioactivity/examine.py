import math

from .config import RadioactivityCard


class RadioactivityCardExamine:
    """检查卡内配置; 半衰期在参数换算前检查, 几何引用由总检查处理"""

    @staticmethod
    def examine(card: RadioactivityCard) -> None:
        groups = ((card.initial.concentration, "initial.concentration", {"uniform_from_inventory"}),
                  (card.boundary, "boundary", {"robin"}),
                  (card.interfaces.interface, "interfaces", {"perfect_contact"}),
                  (card.source.region, "source", {"volumetric_nuclide_birth_rate"}))
        for items, location, models in groups:
            names = [item.region if location == "initial.concentration" else item.name for item in items]
            if len(names) != len(set(names)):
                raise ValueError(f"{location}: 名称不能重复")
            for name, item in zip(names, items):
                if item.model not in models:
                    raise ValueError(f"{location}.{name}: 不支持的模型 {item.model}")
                for value in item.parameters.values():
                    numbers = value if isinstance(value, list) else [value]
                    if not all(math.isfinite(number) for number in numbers):
                        raise ValueError(f"{location}.{name}: 参数值必须有限")
                if location == "initial.concentration":
                    inventory = item.parameters.get("inventory")
                    if not isinstance(inventory, float) or inventory < 0:
                        raise ValueError(f"{location}.{name}: inventory 必须为非负标量, 内部单位为 atom")
