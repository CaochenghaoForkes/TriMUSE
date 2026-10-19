import math

from .config import ThermalCard

class ThermalCardExamine:
    """检查热卡自身, 几何区域与界面引用由卡间检查负责"""

    @classmethod
    def examine(cls, card: ThermalCard) -> None:
        """检查有限参数, 名称重复及已定义模型的基本要求"""
        regions = [temperature.region for temperature in card.initial.temperature]
        if len(set(regions)) != len(regions):
            raise ValueError("initial.temperature: 区域不能重复")
        for temperature in card.initial.temperature:
            location = f"initial.temperature.{temperature.region}"
            cls.check_parameters(temperature.parameters, location)
            if temperature.model == "uniform":
                cls.check_scalar(temperature.parameters, "uniform_value", location, nonnegative=True)
        for items, location in ((card.boundary, "boundary"), (card.interfaces.interface, "interfaces"), (card.source.region, "source")):
            names = [item.name for item in items]
            if len(set(names)) != len(names):
                raise ValueError(f"{location}: 名称不能重复")
            for item in items:
                cls.check_parameters(item.parameters, f"{location}.{item.name}")
        for boundary in card.boundary:
            cls.check_boundary(boundary)

    @staticmethod
    def check_parameters(parameters: dict[str, float | list[float]], location: str) -> None:
        """标量与列表中的所有数值必须有限"""
        for value in parameters.values():
            numbers = value if isinstance(value, list) else [value]
            if not all(math.isfinite(number) for number in numbers):
                raise ValueError(f"{location}: 参数值必须有限")

    @staticmethod
    def check_scalar(parameters: dict[str, float | list[float]], name: str, location: str, *, nonnegative: bool = False) -> None:
        """检查当前常数模型所需的标量参数"""
        value = parameters.get(name)
        if value is None or isinstance(value, list):
            raise ValueError(f"{location}: {name} 必须为标量")
        if nonnegative and value < 0:
            raise ValueError(f"{location}: {name} 不能为负")

    @classmethod
    def check_boundary(cls, boundary: ThermalCard.Boundary) -> None:
        """检查边界模型"""
        location = f"boundary.{boundary.name}"
        if boundary.model not in ("dirichlet", "neumann", "robin"):
            raise ValueError(f"{location}: model 必须为 [dirichlet,neumann,robin]")

