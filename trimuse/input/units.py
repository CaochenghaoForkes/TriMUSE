from __future__ import annotations
import re
from types import MappingProxyType
import pint
import numpy as np

class Units:
    """统一使用一个单位注册表，方法通过类名直接调用"""

    _REGISTRY = pint.UnitRegistry(autoconvert_offset_to_baseunit=False)

    # 常用物理量的 SI 目标单位
    SI_UNITS = MappingProxyType({
        "length": "[m]",
        "angle": "[rad]",
        "mass": "[kg]",
        "time": "[s]",
        "temperature": "[K]",
        "temperature_difference": "[K]",
        "current": "[A]",
        "amount": "[mol]",
        "luminous_intensity": "[cd]",
        "dimensionless": "[1]",
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
    def _parse_unit(unit: str, *, context: str | None = None) -> pint.Unit:
        """解析自定义单位语法, 返回 Pint 单位对象"""
        error_message = f"无效的单位表达式: {unit!r}"
        if not isinstance(unit, str) or not unit.strip():
            raise Units._error(error_message, context)
        text = unit.strip()
        if text.count("/") > 1:
            raise Units._error(error_message, context)
        sides = text.split("/")
        expressions = []
        tokens = []
        for side in sides:
            position = 0
            factors = []
            while position < len(side):
                match = Units._TOKEN.match(side, position)
                if match is None:
                    raise Units._error(error_message, context)
                symbol, power_text = match.groups()
                symbol = Units._ALIASES.get(symbol, symbol)
                power = int(power_text) if power_text is not None else 1
                if symbol == "1":
                    symbol = "dimensionless"
                factors.append(symbol if power == 1 else f"{symbol}**{power}")
                tokens.append((symbol, power))
                position = match.end()
            if not factors:
                raise Units._error(error_message, context)
            expressions.append("*".join(factors))

        # 摄氏度和华氏度只能单独出现, 复合单位使用温差单位或开尔文
        for symbol, power in tokens:
            if symbol in ("degC", "degF", "degree_Celsius", "degree_Fahrenheit", "celsius", "fahrenheit") and (len(tokens) != 1 or len(sides) != 1 or power != 1):
                raise Units._error(error_message, context)
        
        expression = expressions[0] if len(expressions) == 1 else f"({expressions[0]})/({expressions[1]})"
        try:
            return Units._REGISTRY.parse_units(expression, as_delta=False)
        except (pint.errors.PintError, ValueError, TypeError, OverflowError) as exc:
            raise Units._error(error_message, context) from exc

    @staticmethod
    def convert(
        value: float | list[float],
        unit: str,
        target_unit: str,
        *,
        context: str | None = None,
    ) -> float | list[float]:
        """转换标量或一维数值列表, 返回形式与输入一致"""
        is_list = isinstance(value, list)
        number = np.asarray(value, dtype=float)
        source = Units._parse_unit(unit, context=context)
        target = Units._parse_unit(target_unit, context=context)
        try:
            result = np.asarray(Units._REGISTRY.Quantity(number, source).to(target).magnitude, dtype=float)
        except (pint.errors.PintError, ValueError, TypeError, OverflowError) as exc:
            raise Units._error(f"单位 {unit!r} 无法转换为 {target_unit!r}", context) from exc
        return result.tolist() if is_list else float(result)
