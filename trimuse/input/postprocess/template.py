from pathlib import Path

from ..xml_utils import XmlUtils

class PostprocessCardTemplate:
    CONTENT = '''<?xml version="1.0" encoding="UTF-8"?>
<!-- 默认处理全部已保存时刻; 坐标为实际笛卡尔坐标, 名称仅为采样位置标签 -->
<postprocess>
    <!-- 1D 默认输出 100 点径向线; 2D 默认输出全截面; 3D 至少指定一项 -->
    <points>
        <point name="[str]">
            <center>
                <x value="[float]" unit="[um]"/>
                <!-- y 用于 2D/3D, z 用于 3D -->
                <y value="[float]" unit="[um]"/>
                <z value="[float]" unit="[um]"/>
            </center>
        </point>
    </points>
    <lines>
        <!-- sample_count 至少 2, 默认 100; 起点固定为颗粒中心, 沿方向到外表面 -->
        <!-- 1D 为整个径向范围, 可省略 direction -->
        <line name="[str]" sample_count="[int]">
            <!-- 2D theta 从 +y 起算; 3D theta 从 +z 起算, phi 绕 z 轴 -->
            <!-- theta 在 [0,180] 度, phi 在 [0,360) 度; 2D 不需要 phi -->
            <direction>
                <theta value="[float]" unit="[degree]"/>
                <phi value="[float]" unit="[degree]"/>
            </direction>
        </line>
    </lines>
    <!-- 1D 不支持 section; 2D 为整个计算截面, 可省略 center、normal -->
    <sections>
        <section name="[str]">
            <!-- 以下 center、normal 在 3D 必填, 在 2D 不参与截面定义 -->
            <center>
                <x value="[float]" unit="[um]"/>
                <y value="[float]" unit="[um]"/>
                <z value="[float]" unit="[um]"/>
            </center>
            <normal>
                <theta value="[float]" unit="[degree]"/>
                <phi value="[float]" unit="[degree]"/>
            </normal>
        </section>
    </sections>
    <!-- 属性名称使用空格分隔; 当前场输出仅包含 temperature -->
    <properties names="temperature"/>
</postprocess>
'''

    @classmethod
    def write(cls, path: str | Path = "postprocess_empty.xml") -> Path:
        return XmlUtils.write_template(cls.CONTENT, path)
