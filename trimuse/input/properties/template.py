from pathlib import Path

from ..xml_utils import XmlUtils

class PropertiesCardTemplate:
    CONTENT = '''<?xml version="1.0" encoding="UTF-8"?>
<!-- 按 global、材料 region 或外表面 zone 配置属性；zone 名称来自 geometry_mesh 的 surface_zones -->
<properties>
    <external_property name="power">
        <region name="[str]" model="time_series">
            <parameter name="times" value="[float] [float]" unit="[str]" si_unit="[s]"/>
            <parameter name="values" value="[float] [float]" unit="[str]" si_unit="[W]"/>
        </region>
        <!-- 常数模型可用于其他区域, 不使用时删除 -->
        <region name="[str]" model="constant">
            <parameter name="constant_value" value="[float]" unit="[str]" si_unit="[W]"/>
        </region>
    </external_property>
    <external_property name="environment_temperature">
        <zone name="[str]" model="constant">
            <parameter name="constant_value" value="[float]" unit="[str]" si_unit="[K]"/>
        </zone>
        <!-- 另一分区可使用 time_series，times/values 等长且 times 严格递增。
        <zone name="[str]" model="time_series">
            <parameter name="times" value="[float] [float]" unit="[s]" si_unit="[s]"/>
            <parameter name="values" value="[float] [float]" unit="[K]" si_unit="[K]"/>
        </zone>
        -->
    </external_property>
    <!-- Neumann 边界使用同名 zone 的 heat_flux, 单位 W/m²; 正值向外散热, 负值向内加热, 0 为绝热 -->
    <external_property name="heat_flux">
        <zone name="[str]" model="constant">
            <parameter name="constant_value" value="[float]" unit="[W]/[m]^2" si_unit="[W]/[m]^2"/>
        </zone>
        <!-- 时间表模型可替换上面的常数模型, 或用于另一个 zone; times 严格递增, values 可正可负。
        <zone name="[str]" model="time_series">
            <parameter name="times" value="[float] [float]" unit="[s]" si_unit="[s]"/>
            <parameter name="values" value="[float] [float]" unit="[W]/[m]^2" si_unit="[W]/[m]^2"/>
        </zone>
        -->
    </external_property>
    <!-- burnup_percent_fima 使用 0 到 1 的 FIMA 份额, 例如 0.2 表示 20% FIMA -->
    <external_property name="burnup_percent_fima">
        <global model="constant">
            <parameter name="constant_value" value="[float]" unit="[1]" si_unit="[1]"/>
        </global>
        <!-- 如需随时间变化, 用下方节点替换上面的 global 节点
        <global model="time_series">
            <parameter name="times" value="[float] [float]" unit="[s]" si_unit="[s]"/>
            <parameter name="values" value="[float] [float]" unit="[1]" si_unit="[1]"/>
        </global>
        -->
    </external_property>
    <derived_property name="volumetric_power">
        <!-- 需要同一区域已配置外部 power, 计算时由上层提供物理体积 -->
        <region name="[str]" model="uniform"/>
    </derived_property>
    <derived_property name="heat_transfer_coef">
        <zone name="[str]" model="constant">
            <parameter name="constant_value" value="[float]" unit="[str]" si_unit="[W]/[m]^2[K]"/>
        </zone>
        <!-- 时间变化模型可替换上面的常数模型，或用于另一个 zone。
        <zone name="[str]" model="time_series">
            <parameter name="times" value="[float] [float]" unit="[s]" si_unit="[s]"/>
            <parameter name="values" value="[float] [float]" unit="[W]/[m]^2[K]" si_unit="[W]/[m]^2[K]"/>
        </zone>
        -->
    </derived_property>
</properties>
'''

    @classmethod
    def write(cls, path: str | Path = "properties_empty.xml") -> Path:
        return XmlUtils.write_template(cls.CONTENT, path)
