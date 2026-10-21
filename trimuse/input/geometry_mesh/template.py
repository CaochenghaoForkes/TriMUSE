from pathlib import Path
from ..xml_utils import XmlUtils

class GeometryMeshCardTemplate:
    """列出两类几何及全部可选配置, 填写时只保留选定模型"""

    CONTENT = '''<?xml version="1.0" encoding="UTF-8"?>
<!-- 填写占位符后使用, 可选值只选一项, 不使用的可选节点删除 -->
<!-- 长度 unit 使用 [um] 或 [m] 等单位, 无量纲使用 [1] -->
<geometry_mesh>
    <model dimension="[1][2][3]"/>
    <!-- 根据 source 只保留对应的几何子节点 -->
    <geometry source="[ideal_multilayer_sphere][stochastic_multilayer_particle]">
        <ideal_multilayer_sphere>
            <!-- center 可省略, 坐标默认零, 1D 保留 x, 2D 保留 x,y, 3D 保留 x,y,z -->
            <center>
                <x value="[float]" unit="[str]"/>
                <y value="[float]" unit="[str]"/>
                <z value="[float]" unit="[str]"/>
            </center>
            <regions>
                <!-- 按从内到外顺序重复 region, 名称与层数自定, 外半径严格递增 -->
                <!-- outer_boundary 为该层外界面名称, 最后一层为颗粒外表面 -->
                <region name="[str]">
                    <outer_boundary name="[str]"/>
                    <outer_radius value="[float]" unit="[str]"/>
                </region>
            </regions>
        </ideal_multilayer_sphere>
        <stochastic_multilayer_particle>
            <!-- 随机种子在总调度卡中设置, center 规则同理想几何 -->
            <center>
                <x value="[float]" unit="[str]"/>
                <y value="[float]" unit="[str]"/>
                <z value="[float]" unit="[str]"/>
            </center>
            <outer_surface>
                <!-- 最外层材料名称不限, 输入其批次平均外半径 -->
                <batch_mean_radius value="[float]" unit="[str]"/>
                <!-- 1D 删除球形度, 2D 轴对称与 3D 均使用球形度 -->
                <target_sphericity value="[float]" unit="[1]"/>
                <!-- 1D 删除, 2D 和 3D 可省略, 默认 max_degree=30, spectrum_decay=4 -->
                <shape_expansion max_degree="[int]" spectrum_decay="[float]"/>
            </outer_surface>
            <regions>
                <!-- 第一层为核心, 使用半径的批次统计 -->
                <region name="[str]">
                    <outer_boundary name="[str]"/>
                    <batch_mean_radius value="[float]" unit="[str]"/>
                    <batch_radius_std value="[float]" unit="[str]"/>
                </region>
                <!-- 包覆层按从内到外顺序重复, 单层颗粒删除此项 -->
                <region name="[str]">
                    <outer_boundary name="[str]"/>
                    <batch_mean_thickness value="[float]" unit="[str]"/>
                    <batch_thickness_std value="[float]" unit="[str]"/>
                    <!-- 1D 删除以下三项, 2D 和 3D 使用颗粒内角向统计与拆包裹系数 -->
                    <within_particle_thickness_std value="[float]" unit="[str]"/>
                    <within_particle_radius_std value="[float]" unit="[str]"/>
                    <unwrapping_coefficient value="[float]" unit="[1]"/>
                </region>
            </regions>
        </stochastic_multilayer_particle>
        <!-- 必填: 外表面物理边界分区。outer_boundary 引用最外层 outer_boundary 名称。
             三种维数只保留对应的一种写法；不细分时仅填写 residual_zone。 -->
        <!-- 1D: 外端点只能作为 residual_zone。
        <zones outer_boundary="[str]">
            <residual_zone name="[str]"/>
        </zones>
        -->
        <!-- 2D 轴对称: theta 从上极点沿右侧截面至下极点，范围 0 到 180 度。 -->
        <zones outer_boundary="[str]">
            <zone name="[str]">
                <theta_min value="[float]" unit="[degree]"/>
                <theta_max value="[float]" unit="[degree]"/>
            </zone>
            <residual_zone name="[str]"/>
        </zones>
        <!-- 3D: theta/phi 指定中心方向，angular_distance_min/max 定义球面环带；min=0 为球冠。
        <zones outer_boundary="[str]">
            <zone name="[str]">
                <theta value="[float]" unit="[degree]"/>
                <phi value="[float]" unit="[degree]"/>
                <angular_distance_min value="[float]" unit="[degree]"/>
                <angular_distance_max value="[float]" unit="[degree]"/>
            </zone>
            <residual_zone name="[str]"/>
        </zones>
        -->
    </geometry>
    <mesh>
        <!-- 可选: 仅控制随机几何采样, 不改变展开次数或球形度积分规则 -->
        <!-- 两个属性可分别省略, 缺省沿用自动公式; 2D 仅使用 theta_interval_count -->
        <!-- theta_interval_count 至少 2, phi_count 至少 3; 1D 和理想几何不使用 -->
        <space_sampler theta_interval_count="[int]" phi_count="[int]"/>
        <!-- 维数决定线段, 三角形或四面体类型, geometry_order 默认 1 -->
        <element geometry_order="[1][2]"/>
        <sizing>
            <!-- global 的两个尺寸可分别省略 -->
            <global>
                <min_edge_length value="[float]" unit="[str]"/>
                <max_edge_length value="[float]" unit="[str]"/>
            </global>
            <!-- region 可省略或重复, name 引用材料层名称 -->
            <region name="[str]">
                <target_edge_length value="[float]" unit="[str]"/>
            </region>
        </sizing>
        <!-- 固定输出到输出根目录/mesh/<geometry source>.msh 和同名 .vtk -->
        <!-- algorithm_3d 仅 3D 使用, 默认 hxt, optimizer 默认 gmsh -->
        <!-- 输出格式内部固定为 MSH 2.2 ASCII -->
        <gmsh algorithm_3d="[hxt][mmg3d][del3d][front3d]" optimizer="[gmsh][netgen][none]"/>
        <!-- 可选, 空格分隔 Gmsh 支持的格式扩展名; 默认 vtk, MSH 和 VTK 始终输出 -->
        <output formats="vtk bdf"/>
    </mesh>
</geometry_mesh>
'''

    @classmethod
    def write(cls, path: str | Path = "geometry_mesh_empty.xml") -> Path:
        """写出待填写模板, 由外部 loader 调用"""
        return XmlUtils.write_template(cls.CONTENT, path)
