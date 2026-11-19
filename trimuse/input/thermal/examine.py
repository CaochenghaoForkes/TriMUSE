import math

from .config import ThermalCard

class ThermalCardExamine:
    """检查热卡自身, 保留原始位置名称"""

    @classmethod
    def examine(cls, card: ThermalCard) -> None:
        cls.check_models(card.regions.region, {'conduction': {'fourier'}, 'capacity': {'rho_cp'}, 'source': {'volumetric_power'}}, "thermal.regions")
        cls.check_models(card.boundaries.boundary, {'heat_boundary': {'robin', 'dirichlet', 'neumann'}}, "thermal.boundaries")
        cls.check_models(card.interfaces.interface, {'interface_heat_transfer': {'interface_thermal_resistance', 'perfect_contact'}}, "thermal.interfaces")
        for region in card.regions.region:
            if not any(model.name == "conduction" for model in region.model):
                raise ValueError(f"thermal.regions.{region.name}: 缺少导热组装项 conduction")
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

    @staticmethod
    def check_local_resistance(parameters: dict[str, float | list[float]], location: str) -> None:
        """每项定义一块角度区域；单块也接受标量，重叠时按输入顺序优先"""
        planar = "theta_mins" in parameters
        names = ("theta_mins", "theta_maxs") if planar else ("thetas", "phis", "angular_distance_mins", "angular_distance_maxs")
        allowed = {"constant_value", *names}
        if set(parameters) != allowed:
            raise ValueError(f"{location}: 所需参数为 {', '.join(sorted(allowed))}")
        arrays = {name: value if isinstance(value, list) else [value] for name, value in parameters.items()}
        count = len(arrays["constant_value"])
        if count == 0 or any(len(values) != count for values in arrays.values()):
            raise ValueError(f"{location}: 参数列表必须非空且长度一致")
        if any(value <= 0 for value in arrays["constant_value"]):
            raise ValueError(f"{location}: constant_value 必须大于零")
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

    @staticmethod
    def check_models(locations, allowed: dict[str, set[str]], context: str) -> None:
        for location in locations:
            terms = set()
            for model in location.model:
                if model.model_name not in allowed.get(model.name, set()):
                    raise ValueError(f"{context}.{location.name}: 不支持的组装项和模型 {model.name}/{model.model_name}")
                if model.name in terms:
                    raise ValueError(f"{context}.{location.name}: 组装项 {model.name} 重复配置")
                terms.add(model.name)
