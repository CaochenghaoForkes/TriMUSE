import math

from .config import PropertiesCard
from ..thermal.examine import ThermalCardExamine

class PropertiesCardExamine:
    COMMON_MODELS = {"common.constant", "common.time_series"}
    MODELS = {
        "assembly.interface_pressure": {"common.uniform_from_constant_value", "common.local_from_constant_value"},
        "assembly.interface_thermal_resistance": {"common.uniform_from_constant_value", "common.local_from_constant_value"},
        "external.power": COMMON_MODELS,
        "external.energy_per_fission": {"common.constant"},
        "derived.burnup_fima_fraction": COMMON_MODELS | {"common.accumulate_from_fission_rate"},
        "external.uranium_fraction": COMMON_MODELS,
        "external.initial_heavy_metal_inventory": {"common.constant"},
        "external.nuclide_birth_rate": COMMON_MODELS,
        "external.fast_neutron_flux": COMMON_MODELS,
        "external.environment_temperature": COMMON_MODELS,
        "external.surface_temperature": COMMON_MODELS,
        "external.heat_flux": COMMON_MODELS,
        "external.surface_traction": COMMON_MODELS,
        "external.surface_displacement": COMMON_MODELS,
        "external.surface_pressure": COMMON_MODELS,
        "external.environment_nuclide_concentration": COMMON_MODELS,
        "external.surface_nuclide_concentration": COMMON_MODELS,
        "external.nuclide_flux": COMMON_MODELS,
        "derived.heat_transfer_coef": COMMON_MODELS,
        "derived.mass_transfer_coef": COMMON_MODELS,
        "derived.region_volume": {"common.undeformed"},
        "derived.fission_gas_production_rate": COMMON_MODELS | {"kernel.bison_2021"},
        "derived.average_volumetric_fission_rate": COMMON_MODELS | {"common.from_power"},
        "derived.volumetric_power": COMMON_MODELS | {"common.uniform_from_power"},
        "derived.volumetric_nuclide_birth_rate": COMMON_MODELS | {"common.uniform_from_birth_rate"},
        "derived.irradiation_time": {"common.accumulate", "common.accumulate_from_fast_neutron_flux"},
        "derived.fast_neutron_fluence": COMMON_MODELS | {"common.accumulate_from_fast_neutron_flux"},
    }

    MECHANICS_PROPERTIES = {"external.surface_traction", "external.surface_displacement", "external.surface_pressure"}

    RADIOACTIVITY_PROPERTIES = {
        "external.nuclide_birth_rate": ("external_property", "global"),
        "external.fast_neutron_flux": ("external_property", "global"),
        "external.uranium_fraction": ("external_property", "region"),
        "external.environment_nuclide_concentration": ("external_property", "zone"),
        "external.surface_nuclide_concentration": ("external_property", "zone"),
        "external.nuclide_flux": ("external_property", "zone"),
        "derived.mass_transfer_coef": ("derived_property", "zone"),
        "derived.volumetric_nuclide_birth_rate": ("derived_property", "region"),
        "derived.average_volumetric_fission_rate": ("derived_property", "region"),
        "derived.fission_gas_production_rate": ("derived_property", "region"),
        "derived.irradiation_time": ("derived_property", "global"),
        "derived.fast_neutron_fluence": ("derived_property", "global"),
    }

    @classmethod
    def examine(cls, card: PropertiesCard) -> None:
        """检查名称唯一性和当前已实现模型的参数"""
        for category, properties in (
            ("external_property", card.external_property),
            ("derived_property", card.derived_property),
            ("assembly_property", card.assembly_property),
        ):
            names = [property_config.name for property_config in properties]
            if len(names) != len(set(names)):
                raise ValueError(f"{category}: 属性名称不能重复")
            for property_config in properties:
                cls.check_property(category, property_config)

    @classmethod
    def check_property(cls, category: str, property_config: PropertiesCard.Property) -> None:
        location = property_config.name
        if location not in cls.MODELS or not location.startswith(category.removesuffix("_property") + "."):
            raise ValueError(f"{location}: 不支持的 {category} 属性")
        if location == "assembly.interface_pressure":
            cls.check_interface_pressure(property_config)
            return
        if location == "assembly.interface_thermal_resistance":
            cls.check_interface_thermal_resistance(property_config)
            return
        if property_config.interface:
            raise ValueError(f"{location}: 当前属性不支持 interface 配置")
        configs = [*property_config.region, *property_config.zone]
        if property_config.global_ is not None:
            configs.append(property_config.global_)
        for config in configs:
            if config.model not in cls.MODELS[location]:
                raise ValueError(f"{location}: 不支持的模型 {config.model}, 可选 {', '.join(sorted(cls.MODELS[location]))}")
        if not property_config.region and not property_config.zone and property_config.global_ is None:
            raise ValueError(f"{location}: 至少需要一个 region、zone 或 global")
        if property_config.name in ("external.environment_temperature", "external.surface_temperature", "external.heat_flux", "derived.heat_transfer_coef") and (property_config.region or property_config.global_ is not None):
            raise ValueError(f"{location}: 请按 zone 配置边界属性")
        region_names = [region.name for region in property_config.region]
        if len(region_names) != len(set(region_names)):
            raise ValueError(f"{location}: 区域名称不能重复")
        zone_names = [zone.name for zone in property_config.zone]
        if len(zone_names) != len(set(zone_names)):
            raise ValueError(f"{location}: 外表面分区名称不能重复")
        if location == "derived.burnup_fima_fraction":
            if not property_config.region or property_config.zone or property_config.global_ is not None:
                raise ValueError(f"{location}: 请按 region 配置 FIMA 份额")
            for region in property_config.region:
                cls.check_burnup(region, f"{location}.{region.name}")
            return
        if location == "derived.region_volume":
            if not property_config.region or property_config.zone or property_config.global_ is not None:
                raise ValueError(f"{location}: 请按 region 配置当前区域体积")
            for region in property_config.region:
                if region.parameters:
                    raise ValueError(f"{location}.{region.name}: common.undeformed 不需要模型参数")
            return
        if location == "external.initial_heavy_metal_inventory":
            if not property_config.region or property_config.zone or property_config.global_ is not None:
                raise ValueError(f"{location}: 请按 region 配置初始重金属盘存")
            for region in property_config.region:
                cls.check_time_model(region, f"{location}.{region.name}")
                if region.parameters["constant_value"] <= 0.0:
                    raise ValueError(f"{location}.{region.name}: 初始重金属原子总数必须为正数")
            return
        if location == "external.energy_per_fission":
            if property_config.global_ is None or property_config.region or property_config.zone:
                raise ValueError(f"{location}: 请按 global 配置单次裂变释热能量")
            cls.check_time_model(property_config.global_, location)
            if property_config.global_.parameters["constant_value"] <= 0.0:
                raise ValueError(f"{location}: 单次裂变释热能量必须为正数")
            return
        if property_config.name in cls.MECHANICS_PROPERTIES:
            if property_config.region or property_config.global_ is not None:
                raise ValueError(f"{location}: 请按 zone 配置边界属性")
            for zone in property_config.zone:
                if property_config.name == "external.surface_pressure":
                    cls.check_time_model(zone, f"{location}.{zone.name}")
                else:
                    cls.check_vector_time_model(zone, f"{location}.{zone.name}")
            return
        if property_config.name in cls.RADIOACTIVITY_PROPERTIES:
            cls.check_radioactivity_property(category, property_config)
            return
        for zone in property_config.zone:
            zone_location = f"{location}.{zone.name}"
            if (category, property_config.name) not in (("external_property", "external.environment_temperature"), ("external_property", "external.surface_temperature"), ("external_property", "external.heat_flux"), ("derived_property", "derived.heat_transfer_coef")):
                raise ValueError(f"{zone_location}: 当前属性不支持 zone 配置")
            cls.check_time_model(zone, zone_location)
        if property_config.global_ is not None:
            global_config = property_config.global_
            for value in global_config.parameters.values():
                numbers = value if isinstance(value, list) else [value]
                if not all(math.isfinite(number) for number in numbers):
                    raise ValueError(f"{location}.global: 参数值必须有限")
            if global_config.model == "common.constant" and not isinstance(global_config.parameters.get("constant_value"), float):
                raise ValueError(f"{location}.global: constant_value 必须为标量")
        for region in property_config.region:
            region_location = f"{location}.{region.name}"
            for value in region.parameters.values():
                numbers = value if isinstance(value, list) else [value]
                if not all(math.isfinite(number) for number in numbers):
                    raise ValueError(f"{region_location}: 参数值必须有限")
            if category == "external_property":
                if property_config.name != "external.power":
                    raise ValueError(f"{location}: 尚不支持的外部量")
                cls.check_power(region, region_location)
            elif property_config.name == "derived.volumetric_power":
                if region.model in cls.COMMON_MODELS:
                    cls.check_time_model(region, region_location)
                elif region.parameters:
                    raise ValueError(f"{region_location}: common.uniform_from_power 不需要模型参数")
            else:
                raise ValueError(f"{region_location}: 尚不支持的派生量")

    @classmethod
    def check_interface_pressure(cls, property_config: PropertiesCard.Property) -> None:
        location = property_config.name
        if not property_config.interface or property_config.region or property_config.zone or property_config.global_ is not None:
            raise ValueError(f"{location}: 请按 interface 配置界面压力")
        names = [item.name for item in property_config.interface]
        if len(names) != len(set(names)):
            raise ValueError(f"{location}: 界面名称不能重复")
        for interface in property_config.interface:
            context = f"{location}.{interface.name}"
            parameters = interface.parameters
            if interface.model not in cls.MODELS[location]:
                raise ValueError(f"{context}: 不支持的模型 {interface.model}")
            ThermalCardExamine.check_parameters(parameters, context)
            if interface.model == "common.local_from_constant_value":
                ThermalCardExamine.check_local_resistance(parameters, context, allow_zero=True)
            elif set(parameters) != {"constant_value"} or not isinstance(parameters["constant_value"], float) or parameters["constant_value"] < 0:
                raise ValueError(f"{context}: constant_value 必须为非负标量")

    @classmethod
    def check_interface_thermal_resistance(cls, property_config: PropertiesCard.Property) -> None:
        location = property_config.name
        if not property_config.interface or property_config.region or property_config.zone or property_config.global_ is not None:
            raise ValueError(f"{location}: 请按 interface 配置界面热阻")
        names = [item.name for item in property_config.interface]
        if len(names) != len(set(names)):
            raise ValueError(f"{location}: 界面名称不能重复")
        for interface in property_config.interface:
            context = f"{location}.{interface.name}"
            model = interface.model
            parameters = interface.parameters
            if model not in cls.MODELS[location]:
                raise ValueError(f"{context}: 不支持的模型 {model}")
            ThermalCardExamine.check_parameters(parameters, context)
            if model == "common.local_from_constant_value":
                ThermalCardExamine.check_local_resistance(parameters, context)
            elif set(parameters) != {"constant_value"} or not isinstance(parameters["constant_value"], float) or parameters["constant_value"] <= 0:
                raise ValueError(f"{context}: constant_value 必须为正的标量")

    @staticmethod
    def check_radioactivity_property(category: str, property_config: PropertiesCard.Property) -> None:
        """核素属性的分类、范围以及当前标量模型参数"""
        name = property_config.name
        location = name
        expected_category, scope = PropertiesCardExamine.RADIOACTIVITY_PROPERTIES[name]
        if category != expected_category or (property_config.region and scope != "region") or (property_config.zone and scope != "zone") or (property_config.global_ is not None and scope != "global"):
            raise ValueError(f"{location}: 必须使用 {expected_category} 的 {scope} 配置")
        configs = [property_config.global_] if scope == "global" else getattr(property_config, scope)
        for config in configs:
            item_location = f"{location}.{config.name}" if scope != "global" else f"{location}.global"
            if name == "derived.fission_gas_production_rate" and config.model == "kernel.bison_2021":
                if config.parameters:
                    raise ValueError(f"{item_location}: kernel.bison_2021 不需要模型参数")
                continue
            if name == "derived.average_volumetric_fission_rate" and config.model == "common.from_power":
                if config.parameters:
                    raise ValueError(f"{item_location}: from_power 不需要模型参数")
                continue
            if name == "derived.volumetric_nuclide_birth_rate" and config.model == "common.uniform_from_birth_rate":
                if config.parameters:
                    raise ValueError(f"{item_location}: uniform_from_birth_rate 不需要模型参数")
                continue
            if name == "derived.irradiation_time" or (name == "derived.fast_neutron_fluence" and config.model == "common.accumulate_from_fast_neutron_flux"):
                initial_value = config.parameters.get("initial_value")
                if set(config.parameters) != {"initial_value"} or not isinstance(initial_value, float) or not math.isfinite(initial_value) or initial_value < 0.0:
                    raise ValueError(f"{item_location}: 必须指定有限非负标量 initial_value")
                continue
            PropertiesCardExamine.check_time_model(config, item_location)
            values = [config.parameters["constant_value"]] if config.model == "common.constant" else config.parameters["values"]
            if name != "external.nuclide_flux" and any(value < 0 for value in values):
                raise ValueError(f"{item_location}: 属性值不能为负")
            if name == "external.uranium_fraction" and any(value > 1 for value in values):
                raise ValueError(f"{item_location}: 区域产生率份额必须在 [0,1] 内")

    @staticmethod
    def check_power(region: PropertiesCard.Region, location: str) -> None:
        if region.model == "common.constant":
            if not isinstance(region.parameters.get("constant_value"), float):
                raise ValueError(f"{location}: constant_value 必须为标量")
        elif region.model == "common.time_series":
            times = region.parameters.get("times")
            values = region.parameters.get("values")
            times = times if isinstance(times, list) else [times] if times is not None else []
            values = values if isinstance(values, list) else [values] if values is not None else []
            if not times or len(times) != len(values) or any(right <= left for left, right in zip(times, times[1:])):
                raise ValueError(f"{location}: times 和 values 必须等长, times 严格递增")
        else:
            raise ValueError(f"{location}: 不支持的模型 {region.model}")

    @staticmethod
    def check_burnup(config: PropertiesCard.Region, location: str) -> None:
        if config.model == "common.accumulate_from_fission_rate":
            initial_value = config.parameters.get("initial_value")
            if set(config.parameters) != {"initial_value"} or not isinstance(initial_value, float) or not math.isfinite(initial_value) or not 0.0 <= initial_value <= 1.0:
                raise ValueError(f"{location}: 必须指定 [0,1] 内的有限标量 initial_value")
            return
        PropertiesCardExamine.check_time_model(config, location)
        values = [config.parameters["constant_value"]] if config.model == "common.constant" else config.parameters["values"]
        if any(not 0 <= value <= 1 for value in values):
            raise ValueError(f"{location}: FIMA 份额必须在 [0,1] 内")

    @staticmethod
    def check_time_model(config: PropertiesCard.Global | PropertiesCard.Region | PropertiesCard.Zone, location: str) -> None:
        """检查标量常数及时间表模型"""
        for value in config.parameters.values():
            numbers = value if isinstance(value, list) else [value]
            if not all(math.isfinite(number) for number in numbers):
                raise ValueError(f"{location}: 参数值必须有限")
        if config.model == "common.constant":
            if not isinstance(config.parameters.get("constant_value"), float):
                raise ValueError(f"{location}: constant_value 必须为标量")
        elif config.model == "common.time_series":
            times = config.parameters.get("times")
            values = config.parameters.get("values")
            if not isinstance(times, list) or not isinstance(values, list) or len(times) < 2 or len(times) != len(values) or any(right <= left for left, right in zip(times, times[1:])):
                raise ValueError(f"{location}: times 和 values 必须为等长列表, times 至少有两个严格递增的值")
        else:
            raise ValueError(f"{location}: 不支持的模型 {config.model}")

    @staticmethod
    def check_vector_time_model(config: PropertiesCard.Zone, location: str) -> None:
        """向量时间表按时刻依次平铺各分量，具体分量数在卡间检查中与维数核对"""
        for value in config.parameters.values():
            numbers = value if isinstance(value, list) else [value]
            if not all(math.isfinite(number) for number in numbers):
                raise ValueError(f"{location}: 参数值必须有限")
        if config.model == "common.constant":
            value = config.parameters.get("constant_value")
            values = value if isinstance(value, list) else [value]
            if not 1 <= len(values) <= 3 or not all(isinstance(number, float) for number in values):
                raise ValueError(f"{location}: constant_value 必须为 1 到 3 个分量")
        else:
            times = config.parameters.get("times")
            values = config.parameters.get("values")
            if not isinstance(times, list) or len(times) < 2 or any(right <= left for left, right in zip(times, times[1:])):
                raise ValueError(f"{location}: times 必须为至少两个严格递增的时间值")
            if not isinstance(values, list) or len(values) % len(times) != 0 or not 1 <= len(values) // len(times) <= 3:
                raise ValueError(f"{location}: values 必须按时刻平铺，每个时刻包含相同的 1 到 3 个分量")
