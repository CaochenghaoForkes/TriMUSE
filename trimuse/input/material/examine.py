import math

from .config import MaterialCard

class MaterialCardExamine:
    """检查材料卡自身, 几何区域是否存在由卡间检查负责"""

    @classmethod
    def examine(cls, card: MaterialCard) -> None:
        """检查材料定义, 参数数值及分配引用"""
        material_names = [mat.name for mat in card.mat]
        if len(set(material_names)) != len(material_names):
            raise ValueError("材料名称不能重复")
        for mat in card.mat:
            cls.check_mat(mat)
        region_names = [region.name for region in card.assign.region]
        if len(set(region_names)) != len(region_names):
            raise ValueError("同一区域不能重复分配材料")
        for region in card.assign.region:
            if region.mat not in material_names:
                raise ValueError(f"区域 {region.name} 引用了未定义的材料: {region.mat}")

    @classmethod
    def check_mat(cls, mat: MaterialCard.Mat) -> None:
        """每个材料至少定义一项物性, 同一物性不重复配置"""
        if not mat.properties:
            raise ValueError(f"材料 {mat.name} 未定义物性")
        property_names = [property_config.property_name for property_config in mat.properties]
        if len(set(property_names)) != len(property_names):
            raise ValueError(f"材料 {mat.name} 重复配置物性")
        for property_config in mat.properties:
            cls.check_property(property_config, mat.name)

    @staticmethod
    def check_property(property_config: MaterialCard.Property, material_name: str) -> None:
        """检查换算后的参数, 模型构造函数不再重复检查"""
        values = property_config.parameters.values()
        location = f"{material_name}.{property_config.property_name}"
        for value in values:
            numbers = value if isinstance(value, list) else [value]
            if not all(math.isfinite(number) for number in numbers):
                raise ValueError(f"{location}: 参数值必须有限")
        if property_config.property_name == "thermal.conductivity":
            if property_config.model == "common.constant":
                value = property_config.parameters.get("constant_value")
                if value is None or isinstance(value, list) or value <= 0:
                    raise ValueError(f"{location}: constant_value 必须为正标量")
            elif property_config.model == "buffer.bison_2021":
                density = property_config.parameters.get("initial_mass_density")
                if density is None or isinstance(density, list) or not 0 < density < 2250:
                    raise ValueError(f"{location}: initial_mass_density 必须在 (0,2250) kg/m³ 内")
        elif property_config.property_name == "general.porosity":
            if property_config.model == "common.constant":
                value = property_config.parameters.get("constant_value")
                if value is None or isinstance(value, list) or not 0 <= value <= 1:
                    raise ValueError(f"{location}: constant_value 必须为 [0,1] 内的标量")
            elif property_config.model == "common.time_series":
                times = property_config.parameters.get("times")
                values = property_config.parameters.get("values")
                if not isinstance(times, list) or not isinstance(values, list) or len(times) < 2 or len(times) != len(values) or any(right <= left for left, right in zip(times, times[1:])):
                    raise ValueError(f"{location}: times 和 values 必须为等长列表, times 至少有两个严格递增的值")
                if any(not 0 <= value <= 1 for value in values):
                    raise ValueError(f"{location}: values 必须在 [0,1] 内")
            else:
                raise ValueError(f"{location}: 不支持的孔隙率模型 {property_config.model}")
        elif property_config.property_name in ("general.mass_density", "thermal.specific_heat"):
            if property_config.model == "common.constant":
                value = property_config.parameters.get("constant_value")
                if not isinstance(value, float) or value <= 0:
                    raise ValueError(f"{location}: constant_value 必须为正标量")
            elif property_config.model == "common.time_series":
                times = property_config.parameters.get("times")
                values = property_config.parameters.get("values")
                if not isinstance(times, list) or not isinstance(values, list) or len(times) < 2 or len(times) != len(values) or any(right <= left for left, right in zip(times, times[1:])):
                    raise ValueError(f"{location}: times 和 values 必须为等长列表, times 至少有两个严格递增的值")
                if any(value <= 0 for value in values):
                    raise ValueError(f"{location}: values 必须为正数")
            elif property_config.property_name == "thermal.specific_heat" and property_config.model == "kernel.fink_2000_uo2":
                enrichment = property_config.parameters.get("u235_enrichment")
                if not isinstance(enrichment, float) or not 0 <= enrichment <= 1:
                    raise ValueError(f"{location}: u235_enrichment 必须为 [0,1] 内的标量")
            elif property_config.property_name == "thermal.specific_heat" and property_config.model == "sic.bison_2021":
                if property_config.parameters:
                    raise ValueError(f"{location}: sic.bison_2021 不需要构造参数")
            else:
                raise ValueError(f"{location}: 不支持的物性模型 {property_config.model}")
