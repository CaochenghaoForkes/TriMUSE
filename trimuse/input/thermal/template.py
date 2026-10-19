from pathlib import Path

from ..xml_utils import XmlUtils

class ThermalCardTemplate:
    """热输入模板"""

    CONTENT = '''<?xml version="1.0" encoding="UTF-8"?>
<!-- 填写占位符后使用；boundary 名称引用几何卡的外表面 zone 或 residual_zone -->
<!-- si_unit 必须符合模型的内部单位, 参数数组使用空格分隔 -->
<thermal>
    <initial>
        <!-- temperature 可重复, region 引用几何区域, 每个区域填写一次 -->
        <temperature region="[str]" model="uniform">
            <parameter name="uniform_value" value="[float]" unit="[str]" si_unit="[K]"/>
        </temperature>
    </initial>
    <!-- 每个外表面分区配置一个边界; robin 使用同名 zone 的 environment_temperature 和 heat_transfer_coef -->
    <!-- neumann 使用 properties 卡同名 zone 的 heat_flux, 外流为正、流入为负; 此处不重复输入模型参数 -->
    <boundaries>
        <boundary name="[str]" model="neumann"/>
        <boundary name="[str]" model="robin"/>
    </boundaries>
    <!-- interfaces 可省略, 默认理想接触, interface 可重复 -->
    <interfaces>
        <interface name="[str]" model="perfect_contact"/>
    </interfaces>
    <!-- source 可省略, 区域热源模型负责提供 W/m³ -->
    <source>
        <!-- 当前 volumetric_power 热源读取 properties 卡中同区域的派生量 -->
        <region name="[str]" model="volumetric_power"/>
    </source>
</thermal>
'''

    @classmethod
    def write(cls, path: str | Path = "thermal_empty.xml") -> Path:
        """写出待填写的热模板"""
        return XmlUtils.write_template(cls.CONTENT, path)
