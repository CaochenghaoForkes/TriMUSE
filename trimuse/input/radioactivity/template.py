from pathlib import Path

from ..xml_utils import XmlUtils


class RadioactivityCardTemplate:
    """单核素扩散与衰变输入模板"""

    CONTENT = '''<?xml version="1.0" encoding="UTF-8"?>
<radioactivity>
    <!-- 一张卡只处理一个核素; 半衰期换算为 s, 不考虑衰变链 -->
    <nuclide name="Cs137">
        <half_life value="30.05" unit="[year]"/>
    </nuclide>
    <initial>
        <!-- 每个几何 region 填写一次; inventory 是该区域总盘存, 初始化时除以区域体积 -->
        <!-- unit 支持 atom、mol、Bq、Ci 及前缀单位; si_unit 固定为 [atom] -->
        <concentration region="[str]" model="uniform_from_inventory">
            <parameter name="inventory" value="[float]" unit="[atom]" si_unit="[atom]"/>
        </concentration>
    </initial>
    <!-- 每个外表面 zone 配置一个边界; robin 读取 properties 卡同名 zone 的
         environment_nuclide_concentration (atom/m³) 和 mass_transfer_coef (m/s) -->
    <!-- dirichlet 读取 surface_nuclide_concentration (atom/m³);
         neumann 读取 nuclide_flux (atom/(m²·s)), 外流为正、流入为负, 0 为无通量 -->
    <boundaries>
        <boundary name="[str]" model="robin"/>
        <!-- 可将 robin 替换为 dirichlet 或 neumann, 同一 zone 只配置一次 -->
    </boundaries>
    <!-- 可省略, 默认连续空间下的理想接触 -->
    <interfaces>
        <interface name="[str]" model="perfect_contact"/>
    </interfaces>
    <!-- 可省略; 只在明确列出的区域添加源项 -->
    <source>
        <region name="[str]" model="volumetric_nuclide_birth_rate"/>
    </source>
</radioactivity>
'''

    @classmethod
    def write(cls, path: str | Path = "radioactivity_empty.xml") -> Path:
        return XmlUtils.write_template(cls.CONTENT, path)
