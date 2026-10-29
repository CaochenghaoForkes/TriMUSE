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
            if temperature.model != "uniform":
                raise ValueError(f"{location}: model 必须为 uniform")
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
        for item in card.source.region:
            if item.model != "volumetric_power":
                raise ValueError(f"source.{item.name}: model 必须为 volumetric_power")
        for item in card.interfaces.interface:
            location = f"interfaces.{item.name}"
            if item.model not in ("perfect_contact", "uniform_from_constant_resistance", "local_from_constant_resistance"):
                raise ValueError(f"{location}: model 必须为 [perfect_contact,uniform_from_constant_resistance,local_from_constant_resistance]")
            if item.model == "uniform_from_constant_resistance":
                cls.check_scalar(item.parameters, "constant_resistance", location, nonnegative=True)
                if item.parameters["constant_resistance"] == 0:
                    raise ValueError(f"{location}: constant_resistance 必须大于零, 零热阻请使用 perfect_contact")
            elif item.model == "local_from_constant_resistance":
                cls.check_local_resistance(item.parameters, location)

    @staticmethod
    def check_local_resistance(parameters: dict[str, float | list[float]], location: str) -> None:
        """每项定义一块角度区域；单块也接受标量，重叠时按输入顺序优先。"""
        planar = "theta_mins" in parameters
        names = ("theta_mins", "theta_maxs") if planar else ("thetas", "phis", "angular_distance_mins", "angular_distance_maxs")
        allowed = {"constant_resistances", *names}
        if set(parameters) != allowed:
            raise ValueError(f"{location}: 所需参数为 {', '.join(sorted(allowed))}")
        arrays = {name: value if isinstance(value, list) else [value] for name, value in parameters.items()}
        count = len(arrays["constant_resistances"])
        if count == 0 or any(len(values) != count for values in arrays.values()):
            raise ValueError(f"{location}: 参数列表必须非空且长度一致")
        if any(value <= 0 for value in arrays["constant_resistances"]):
            raise ValueError(f"{location}: constant_resistances 必须大于零")
        minimums, maximums = (arrays[name] for name in names[-2:])
        if any(not 0 <= lower < upper <= math.pi for lower, upper in zip(minimums, maximums)):
            raise ValueError(f"{location}: 角范围必须满足 0 ≤ min < max ≤ π，si_unit 使用 [rad]")
        if not planar and (any(not 0 <= value <= math.pi for value in arrays["thetas"])
                           or any(not 0 <= value < 2 * math.pi for value in arrays["phis"])):
            raise ValueError(f"{location}: theta 须在 [0,π]，phi 须在 [0,2π)，si_unit 使用 [rad]")

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

