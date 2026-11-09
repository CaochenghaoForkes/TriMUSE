import math

from .config import MechanicsCard

class MechanicsCardExamine:
    @staticmethod
    def examine(card: MechanicsCard) -> None:
        temperature = card.thermal_expansion_reference_temperature
        if not math.isfinite(temperature) or temperature < 0:
            raise ValueError("热膨胀参考温度必须为不低于 0 K 的有限值")
