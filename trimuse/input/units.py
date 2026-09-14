"""物理量单位解析与转换

相邻方括号单位表示相乘，最多一个斜杠，斜杠后全部属于分母
单位幂次使用整数，不接受乘号和括号
基本量纲及换算关系由 Pint 管理，内部目标单位集中定义为类常量
"""

from __future__ import annotations
import math
import re
from numbers import Real
from types import MappingProxyType
import pint

class Units:
    """统一使用一个单位注册表，方法通过类名直接调用"""

    _REGISTRY = pint.UnitRegistry(autoconvert_offset_to_baseunit=False)

    # 七个基本量的内部单位，运行中不修改
    LENGTH = "[m]"
    MASS = "[kg]"
    TIME = "[s]"
    TEMPERATURE = "[K]"
    CURRENT = "[A]"
    AMOUNT = "[mol]"
    LUMINOUS_INTENSITY = "[cd]"
    DIMENSIONLESS = "[1]"

    # 常用物理量的 SI 目标单位
    SI_UNITS = MappingProxyType({
        "length": LENGTH,
        "mass": MASS,
        "time": TIME,
        "temperature": TEMPERATURE,
        "temperature_difference": TEMPERATURE,
        "current": CURRENT,
        "amount": AMOUNT,
        "luminous_intensity": LUMINOUS_INTENSITY,
        "dimensionless": DIMENSIONLESS,
        "area": "[m]^2",
        "volume": "[m]^3",
        "velocity": "[m]/[s]",
        "density": "[kg]/[m]^3",
        "pressure": "[Pa]",
        "energy": "[J]",
        "power": "[W]",
        "diffusivity": "[m]^2/[s]",
        "thermal_conductivity": "[W]/[m][K]",
        "specific_heat": "[J]/[kg][K]",
        "heat_transfer_coefficient": "[W]/[m]^2[K]",
        "thermal_expansion_coefficient": "[1]/[K]",
    })

    _TOKEN = re.compile(r"\[([A-Za-zµμ°_][A-Za-z0-9µμ°_]*|1)\](?:\^([+-]?[0-9]+))?")
    # 输入中的 C 和 F 专指摄氏度和华氏度，库仑和法拉须使用完整英文名称
    _ALIASES = {
        "µm": "um",
        "μm": "um",
        "C": "degC",
        "°C": "degC",
        "K": "kelvin",
        "F": "degF",
        "°F": "degF",
    }

    @staticmethod
    def _error(message: str, context: str | None = None) -> ValueError:
        """构造带有输入位置的错误信息"""
        return ValueError(f"{context}: {message}" if context else message)

    @staticmethod
    def normalize_unit(unit: str, *, context: str | None = None) -> str:
        """严格解析自定义语法，返回 Pint 能识别的单位表达式"""
        if not isinstance(unit, str) or not unit.strip():
            raise Units._error("单位必须是非空字符串", context)
        text = unit.strip()
        if text.count("/") > 1:
            raise Units._error(f"单位最多包含一个斜杠：{unit}", context)
        sides = text.split("/")
        expressions = []
        tokens = []
        for side in sides:
            position = 0
            factors = []
            while position < len(side):
                match = Units._TOKEN.match(side, position)
                if match is None:
                    raise Units._error(
                        f"单位格式错误：{unit}；请使用相邻方括号单位和整数幂次，"
                        "不使用空格、乘号或括号",
                        context,
                    )
                symbol, power_text = match.groups()
                symbol = Units._ALIASES.get(symbol, symbol)
                power = int(power_text) if power_text is not None else 1
                if symbol == "1":
                    symbol = "dimensionless"
                factors.append(symbol if power == 1 else f"{symbol}**{power}")
                tokens.append((symbol, power))
                position = match.end()
            if not factors:
                raise Units._error(f"单位的分子和分母不能为空：{unit}", context)
            expressions.append("*".join(factors))

        # 偏移温度只能单独出现，复合单位明确使用温差单位或开尔文
        for symbol, power in tokens:
            try:
                atomic = Units._REGISTRY.parse_units(symbol, as_delta=False)
                zero = Units._REGISTRY.Quantity(0, atomic).to_base_units().magnitude
            except (pint.errors.PintError, ValueError, TypeError, OverflowError) as exc:
                raise Units._error(f"无法识别单位 {symbol}: {exc}", context) from exc
            if zero != 0 and (len(tokens) != 1 or len(sides) != 1 or power != 1):
                raise Units._error(
                    f"偏移温度单位 {symbol} 只能单独使用；"
                    "温差请使用 delta_degC、delta_degF 或 K",
                    context,
                )
        if len(expressions) == 1:
            return expressions[0]
        return f"({expressions[0]})/({expressions[1]})"

    @staticmethod
    def parse_unit(unit: str, *, context: str | None = None) -> pint.Unit:
        """解析单位并返回同一注册表中的 Pint 单位对象"""
        expression = Units.normalize_unit(unit, context=context)
        try:
            return Units._REGISTRY.parse_units(expression, as_delta=False)
        except (pint.errors.PintError, ValueError, TypeError, OverflowError) as exc:
            raise Units._error(f"单位表达式无法解析：{unit}: {exc}", context) from exc

    @staticmethod
    def check_dimension(
        unit: str, target_unit: str, *, context: str | None = None
    ) -> None:
        """比较输入与目标单位的量纲，不执行数值转换"""
        source = Units.parse_unit(unit, context=context)
        target = Units.parse_unit(target_unit, context=context)
        if source.dimensionality != target.dimensionality:
            raise Units._error(
                f"量纲不匹配：{unit} 的量纲为 {source.dimensionality}, "
                f"目标 {target_unit} 的量纲为 {target.dimensionality}",
                context,
            )

    @staticmethod
    def convert(
        value: float,
        unit: str,
        target_unit: str,
        *,
        context: str | None = None,
    ) -> float:
        """转换有限标量并返回浮点数，目标单位同时规定期望量纲"""
        if isinstance(value, bool) or not isinstance(value, Real):
            raise Units._error("物理量数值必须为实数标量", context)
        try:
            number = float(value)
        except (ValueError, OverflowError) as exc:
            raise Units._error("物理量数值超出浮点数范围", context) from exc
        if not math.isfinite(number):
            raise Units._error("物理量数值必须有限", context)
        Units.check_dimension(unit, target_unit, context=context)
        source = Units.parse_unit(unit, context=context)
        target = Units.parse_unit(target_unit, context=context)
        try:
            result = float(Units._REGISTRY.Quantity(number, source).to(target).magnitude)
        except (pint.errors.PintError, ValueError, TypeError, OverflowError) as exc:
            raise Units._error(f"单位转换失败：{unit} 转换为 {target_unit}; {exc}", context) from exc
        if not math.isfinite(result):
            raise Units._error("换算结果超出有限数值范围", context)
        return result

    @staticmethod
    def to_si(
        value: float,
        unit: str,
        quantity: str,
        *,
        context: str | None = None,
    ) -> float:
        """根据物理量名称选择固定 SI 目标单位并转换"""
        if quantity not in Units.SI_UNITS:
            raise Units._error(f"未定义内部目标单位的物理量：{quantity}", context)
        return Units.convert(value, unit, Units.SI_UNITS[quantity], context=context)
