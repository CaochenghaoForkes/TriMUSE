import math

from .config import PropertiesCard

class PropertiesCardExamine:
    @classmethod
    def examine(cls, card: PropertiesCard) -> None:
        """检查名称唯一性和当前已实现模型的参数"""
        for category, properties in (("external_property", card.external_property), ("derived_property", card.derived_property)):
            names = [property_config.name for property_config in properties]
            if len(names) != len(set(names)):
                raise ValueError(f"{category}: 属性名称不能重复")
            for property_config in properties:
                cls.check_property(category, property_config)

    @classmethod
    def check_property(cls, category: str, property_config: PropertiesCard.Property) -> None:
        location = f"{category}.{property_config.name}"
        if not property_config.region and not property_config.zone and property_config.global_ is None:
            raise ValueError(f"{location}: 至少需要一个 region、zone 或 global")
        if property_config.name in ("environment_temperature", "surface_temperature", "heat_flux", "heat_transfer_coef") and (property_config.region or property_config.global_ is not None):
            raise ValueError(f"{location}: 请按 zone 配置边界属性")
        region_names = [region.name for region in property_config.region]
        if len(region_names) != len(set(region_names)):
            raise ValueError(f"{location}: 区域名称不能重复")
        zone_names = [zone.name for zone in property_config.zone]
        if len(zone_names) != len(set(zone_names)):
            raise ValueError(f"{location}: 外表面分区名称不能重复")
        for zone in property_config.zone:
            zone_location = f"{location}.{zone.name}"
            if (category, property_config.name) not in (("external_property", "environment_temperature"), ("external_property", "surface_temperature"), ("external_property", "heat_flux"), ("derived_property", "heat_transfer_coef")):
                raise ValueError(f"{zone_location}: 当前属性不支持 zone 配置")
            cls.check_time_model(zone, zone_location)
        if property_config.global_ is not None:
            global_config = property_config.global_
            for value in global_config.parameters.values():
                numbers = value if isinstance(value, list) else [value]
                if not all(math.isfinite(number) for number in numbers):
                    raise ValueError(f"{location}.global: 参数值必须有限")
            if global_config.model == "constant" and not isinstance(global_config.parameters.get("constant_value"), float):
                raise ValueError(f"{location}.global: constant_value 必须为标量")
            if category == "external_property" and property_config.name == "burnup_fima_fraction":
                cls.check_burnup(global_config, f"{location}.global")
        for region in property_config.region:
            region_location = f"{location}.{region.name}"
            for value in region.parameters.values():
                numbers = value if isinstance(value, list) else [value]
                if not all(math.isfinite(number) for number in numbers):
                    raise ValueError(f"{region_location}: 参数值必须有限")
            if category == "external_property":
                if property_config.name != "power":
                    raise ValueError(f"{location}: 尚不支持的外部量")
                cls.check_power(region, region_location)
            elif property_config.name != "volumetric_power" or region.model != "uniform":
                raise ValueError(f"{region_location}: 尚不支持的派生量或模型")

    @staticmethod
    def check_power(region: PropertiesCard.Region, location: str) -> None:
        if region.model == "constant":
            if not isinstance(region.parameters.get("constant_value"), float):
                raise ValueError(f"{location}: constant_value 必须为标量")
        elif region.model == "time_series":
            times = region.parameters.get("times")
            values = region.parameters.get("values")
            times = times if isinstance(times, list) else [times] if times is not None else []
            values = values if isinstance(values, list) else [values] if values is not None else []
            if not times or len(times) != len(values) or any(right <= left for left, right in zip(times, times[1:])):
                raise ValueError(f"{location}: times 和 values 必须等长, times 严格递增")
        else:
            raise ValueError(f"{location}: 不支持的模型 {region.model}")

    @staticmethod
    def check_burnup(config: PropertiesCard.Global, location: str) -> None:
        if config.model == "constant":
            value = config.parameters.get("constant_value")
            if not isinstance(value, float) or not 0 <= value <= 1:
                raise ValueError(f"{location}: constant_value 必须为 [0,1] 内的标量")
        elif config.model == "time_series":
            times = config.parameters.get("times")
            values = config.parameters.get("values")
            if not isinstance(times, list) or not isinstance(values, list) or len(times) < 2 or len(times) != len(values) or any(right <= left for left, right in zip(times, times[1:])):
                raise ValueError(f"{location}: times 和 values 必须为等长列表, times 至少有两个严格递增的值")
            if any(not 0 <= value <= 1 for value in values):
                raise ValueError(f"{location}: values 必须在 [0,1] 内")
        else:
            raise ValueError(f"{location}: 不支持的模型 {config.model}")

    @staticmethod
    def check_time_model(config: PropertiesCard.Zone, location: str) -> None:
        """检查外表面分区当前支持的常数及时间表模型"""
        for value in config.parameters.values():
            numbers = value if isinstance(value, list) else [value]
            if not all(math.isfinite(number) for number in numbers):
                raise ValueError(f"{location}: 参数值必须有限")
        if config.model == "constant":
            if not isinstance(config.parameters.get("constant_value"), float):
                raise ValueError(f"{location}: constant_value 必须为标量")
        elif config.model == "time_series":
            times = config.parameters.get("times")
            values = config.parameters.get("values")
            if not isinstance(times, list) or not isinstance(values, list) or len(times) < 2 or len(times) != len(values) or any(right <= left for left, right in zip(times, times[1:])):
                raise ValueError(f"{location}: times 和 values 必须为等长列表, times 至少有两个严格递增的值")
        else:
            raise ValueError(f"{location}: 不支持的模型 {config.model}")
