from pathlib import Path

from ..xml_utils import XmlUtils

class MaterialCardTemplate:
    """材料卡模板, 材料定义和区域分配分别填写"""

    CONTENT = '''<?xml version="1.0" encoding="UTF-8"?>
<material>
    <!-- mat 可重复, 材料名称自定, 同一材料可被多个区域引用 -->
    <mat name="[str]">
        <thermal>
            <!-- model 填写注册的模型名称, 无参数时删除 parameter 子节点 -->
            <!-- 参数统一使用子节点, 参数名不可重复 -->
            <!-- common.constant 的参数名为 constant_value -->
            <conductivity model="[str]">
                <!-- parameter 可重复, unit 为输入单位, si_unit 必须符合模型要求的内部单位 -->
                <!-- common.constant 可只保留一个名为 constant_value 的 parameter -->
                <!-- 无量纲参数的 unit 和 si_unit 均填写 [1] -->
                <parameter name="[str]" value="[float]" unit="[str]" si_unit="[str]"/>
            </conductivity>
            <specific_heat model="common.constant">
                <parameter name="constant_value" value="[float]" unit="[J]/[kg][K]" si_unit="[J]/[kg][K]"/>
            </specific_heat>
            <!-- 如需随时间变化, 用下方节点替换上面的 specific_heat 节点
            <specific_heat model="common.time_series">
                <parameter name="times" value="[float] [float]" unit="[s]" si_unit="[s]"/>
                <parameter name="values" value="[float] [float]" unit="[J]/[kg][K]" si_unit="[J]/[kg][K]"/>
            </specific_heat>
            -->
            <!-- UO2 核心可用温度相关模型; 富集度为 0 到 1 的比例, 15.5% 填 0.155
            <specific_heat model="kernel.fink_2000_uo2">
                <parameter name="u235_enrichment" value="0.155" unit="[1]" si_unit="[1]"/>
            </specific_heat>
            -->
            <!-- SiC 可用温度相关模型, 不需要构造参数
            <specific_heat model="sic.bison_2021"/>
            -->
        </thermal>
        <!-- 不需要通用物性时可删除 general 节点 -->
        <general>
            <porosity model="common.constant">
                <parameter name="constant_value" value="[float]" unit="[1]" si_unit="[1]"/>
            </porosity>
            <!-- 如需随时间变化, 用下方节点替换上面的 porosity 节点
            <porosity model="common.time_series">
                <parameter name="times" value="[float] [float]" unit="[s]" si_unit="[s]"/>
                <parameter name="values" value="[float] [float]" unit="[1]" si_unit="[1]"/>
            </porosity>
            -->
            <mass_density model="common.constant">
                <parameter name="constant_value" value="[float]" unit="[kg]/[m]^3" si_unit="[kg]/[m]^3"/>
            </mass_density>
            <!-- 如需随时间变化, 用下方节点替换上面的 mass_density 节点
            <mass_density model="common.time_series">
                <parameter name="times" value="[float] [float]" unit="[s]" si_unit="[s]"/>
                <parameter name="values" value="[float] [float]" unit="[kg]/[m]^3" si_unit="[kg]/[m]^3"/>
            </mass_density>
            -->
        </general>
    </mat>
    <assign>
        <!-- region 可重复, name 引用几何区域, mat 引用本卡材料 -->
        <region name="[str]" mat="[str]"/>
    </assign>
</material>
'''

    @classmethod
    def write(cls, path: str | Path = "material_empty.xml") -> Path:
        """写出待填写的材料模板"""
        return XmlUtils.write_template(cls.CONTENT, path)
