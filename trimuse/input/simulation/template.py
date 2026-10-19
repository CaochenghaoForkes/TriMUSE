from pathlib import Path

from ..xml_utils import XmlUtils

class SimulationCardTemplate:
    """初版分阶段仿真配置模板"""

    CONTENT = '''<?xml version="1.0" encoding="UTF-8"?>
<simulation>
    <problem>
        <!-- 各阶段持续时间, 从零开始累计, 不是阶段结束时刻 -->
        <time_span value="[float] [float]" unit="[str]"/>
        <!-- 各阶段公共时间步, 所有场在同一时间点求解 -->
        <min_time_step value="[float] [float]" unit="[str]"/>
        <!-- field 可重复, steady 表示在调度时刻重新求解的准稳态 -->
        <!-- theta 仅用于 transient, steady 时删除该属性; 0 前向, 0.5 Crank-Nicolson, 1 后向; 省略时默认 1 -->
        <field name="[str]" model="[steady][transient]" theta="[0][0.5][1]"/>
    </problem>
    <solver>
        <!-- 全部场共用域与边界求积规则, 两个阶次均为必填 -->
        <quadrature domain_order="[int]" boundary_order="[int]"/>
        <!-- 按 name 关联问题中的场, field 可重复 -->
        <field name="[str]">
            <!-- 默认 H1 一阶, 几何网格阶次与场阶次分别设置 -->
            <fem_space family="H1" order="[int]"/>
            <!-- 初版默认 cg, none, 1e-8, 1000, 可省略相应属性或整个节点 -->
            <linear_solver method="cg" preconditioner="none" relative_tolerance="[float]" max_iterations="[int]"/>
            <!-- Picard 容差比较相邻两轮温度, 绝对容差使用温差单位 -->
            <nonlinear_solver method="picard" relative_tolerance="[float]" max_iterations="[int]">
                <absolute_tolerance value="[float]" unit="[K]"/>
            </nonlinear_solver>
        </field>
    </solver>
    <!-- exchange_interval 为各阶段的基础步数 -->
    <coupling exchange_interval="[int] [int]">
        <!-- step 和 fields 均按书写顺序执行, 组内及跨步的时间推进规则后续完善 -->
        <!-- single 单次求解, staggered 顺序迭代, 迭代参数默认 20 和 1e-6 -->
        <step fields="[str]" method="[single][staggered]" max_iterations="[int]" relative_tolerance="[float]"/>
    </coupling>
    <output>
        <!-- 每个阶段填写一个输出间隔; 初始时刻和各阶段终点额外保存 -->
        <!-- 当前仅保存 thermal/temperature, 时间统一为 s -->
        <!-- 正数: 每隔指定模拟时间输出, unit 仅作用于正数; 负整数: 每隔其绝对值个时间步输出 -->
        <!-- 禁止零; 未落在步末的目标时间在首次达到或超过时输出, 不改变求解步长 -->
        <write_interval value="[float] [float]" unit="[s]"/>
    </output>
</simulation>
'''

    @classmethod
    def write(cls, path: str | Path = "simulation_empty.xml") -> Path:
        """写出待填写的仿真模板"""
        return XmlUtils.write_template(cls.CONTENT, path)
