from __future__ import annotations
import re
from types import MappingProxyType
import pint
import numpy as np

class Units:
    """统一使用一个单位注册表，方法通过类名直接调用"""

    _REGISTRY = pint.UnitRegistry(autoconvert_offset_to_baseunit=False)
    _REGISTRY.define("atom = [particle_number]")

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
    def _normalize_nuclide_unit(
        unit: str,
        decay_constant: float,
        *,
        context: str | None = None,
    ) -> tuple[str, float]:
        """将分子中的活度或物质的量改写为 atom, 返回新单位和数值倍率; 衰变常数单位为 s⁻¹"""
        factor = 1.0
        count = 0
        slash = unit.find("/")

        def replace(match: re.Match) -> str:
            nonlocal factor, count
            symbol, power = match.groups() # 匹配单位与单位的次幂
            if symbol == "1": # 无量纲数不处理
                return match.group()
            atomic = Units._parse_unit(f"[{symbol}]", context=context) # 解析为规范名称 Ci->curie kBq->kilobecquerel
            canonical = str(atomic)
            is_activity = canonical.endswith(("becquerel", "curie")) # 判断是否为活度单位
            is_amount = atomic.dimensionality == Units._REGISTRY.mol.dimensionality # 判断是否为物质的量单位
            if not (symbol == "atom" or is_activity or is_amount):
                return match.group() # 如果是普通单位则保持原样
            count += 1
            if count > 1 or (slash >= 0 and match.start() > slash) or (power is not None and int(power) != 1): # 约定核素单位只能出现一次 只能位于分子 指数只能为1
                raise Units._error("核素单位仅支持在分子中出现一次且指数为 1", context)
            quantity = Units._REGISTRY.Quantity(1.0, atomic) # 创建一个带单位的数值对象，取数值为1，单位为atomic
            if is_activity:
                factor = quantity.to("Bq").magnitude / decay_constant # 取出转为 Bq 的数值 再除 decay_constant
            elif is_amount:
                factor = quantity.to("mol").magnitude * 6.02214076e23 # 同理
            return "[atom]"

        expression = Units._TOKEN.sub(replace, unit) # 在 unit 中找到每一个符合 _TOKEN 的单位片段，逐个调用 replace(match)，用它返回的字符串替换原片段
        return expression, factor

    @staticmethod
    def convert(
        value: float | list[float],
        unit: str,
        target_unit: str,
        *,
        decay_constant: float | None = None,
        context: str | None = None,
    ) -> float | list[float]:
        """转换标量或列表; 提供衰变常数(s⁻¹)时启用活度、mol 与 atom 的换算"""
        is_list = isinstance(value, list)
        number = np.asarray(value, dtype=float)
        source_unit, destination_unit = unit, target_unit
        source_factor = target_factor = 1.0
        if decay_constant is not None:
            if not np.isfinite(decay_constant) or decay_constant <= 0:
                raise Units._error("衰变常数必须为正有限数, 单位为 s⁻¹", context)
            source_unit, source_factor = Units._normalize_nuclide_unit(unit, decay_constant, context=context)
            destination_unit, target_factor = Units._normalize_nuclide_unit(target_unit, decay_constant, context=context)
        source = Units._parse_unit(source_unit, context=context)
        target = Units._parse_unit(destination_unit, context=context)
        try:
            result = np.asarray(Units._REGISTRY.Quantity(number, source).to(target).magnitude, dtype=float)
            result *= source_factor / target_factor
        except (pint.errors.PintError, ValueError, TypeError, OverflowError) as exc:
            raise Units._error(f"单位 {unit!r} 无法转换为 {target_unit!r}", context) from exc
        return result.tolist() if is_list else float(result)
