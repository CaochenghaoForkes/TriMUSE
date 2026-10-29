# Radioactivity

一张输入卡求解一个核素，内部浓度使用 atom/m³；初始盘存为 atom，体积源为 atom/(m³·s)。
不考虑衰变链。物理方程为：

```
∂c/∂t - div(D grad(c)) + lambda*c = S
```

## 阅读顺序

1. `common/radioactivity_domain_model.hpp`：区域配置及模型基类，模型返回积分点上的系数。
2. `common/radioactivity_domain_manager.*`：模型分配、依赖登记、系数更新及 dirty 标记。
3. `diffusion/`、`capacity/`、`source/`：分别注册 `fick`、`unit`、`volumetric_nuclide_birth_rate`。
   扩散读取材料扩散系数；容量为 1；源读取派生体积产生率。域内配置统一使用 `RadioactivityDomainConfig`。
4. `boundary/`：Dirichlet 返回表面浓度；Neumann 返回负的外向通量；Robin 返回 h_m 与 h_m*c_infinity。
5. `interface/`：配置与模型注册，目前只有 `perfect_contact`；未配置的内部材料界面也使用理想接触。
6. `initial/`：区域盘存除以实际体积；连续场的阶跃初值做几何加权 L2 投影。
7. `radioactivity_assembly.*`：按区域/外边界缓存 M、K、F，处理指定浓度和界面约束。
8. `radioactivity_problem.*`：初值、theta 时间推进、Picard 迭代与收敛提交。

## 与 thermal 共用的部分

使用 `physics/common/` 中的 `TrialField`、`DofConstraints`、`InterfaceContext`、
`ScalarIntegrator`、`AssemblyCache` 和求和函数。具体物理模型、缓存失效判断和时间调度仍在本模块。

`ScalarIntegrator` 始终从原网格取得几何与积分点编号，通过试探场取得连续/分域的有限元与自由度。
浓度使用标量 H1 场。属性系统通过 `FieldView` 读取对应区域场，无需改变积分点属性的存储。

## 组装与时间推进

固定网格和空间下，单位容量矩阵 M 在初始化时按区域组装一次；衰变矩阵直接使用 lambda*M。
扩散和源按 region 缓存，边界矩阵与边界载荷按 zone 分别缓存。
`update` 发现系数变化后置 dirty；`assembled` 清除标记；`required_no_change` 用于决定是否复用。
各部分的总和也只在贡献变化时更新。初值投影复用 M。

```
K = Kdiffusion + KRobin + lambda*M
F = Fsource + Fboundary
A = M/dt + theta*K_{n+1}
b = M*c_n/dt + (1-theta)*(F_n-K_n*c_n) + theta*F_{n+1}
```

稳态直接使用 K*c=F。theta=1 无需旧属性；theta=0 使用旧时刻的扩散、衰变和自然边界，
Dirichlet 仍取新时刻浓度。M 恒定，无需混合新旧容量矩阵。
缓存和 State 共享只读矩阵；消元作用于工作副本，更新时替换指针，不污染旧时刻状态。
收敛后同步正式浓度及其耦合材料属性；上一时刻的属性快照仍由外部调度提交。
`assembly_counts()` 返回按区域/边界累计的组装次数，每次求解的信息同时记录增量。

## 分域浓度

simulation 卡的浓度求解器可设置：

```xml
<field name="concentration">
    <fem_space family="H1" order="2" continuity="region"/>
    <!-- 其余求解器配置沿用原格式 -->
</field>
```

省略 continuity 默认 continuous。分域场使用固定、协调网格，各区域采用相同 H1 阶次。
radioactivity 卡的接口保持：

```xml
<interfaces>
    <interface name="interface_1" model="perfect_contact"/>
</interfaces>
```

理想接触通过 P 连接两侧浓度自由度，求解 P^T*A*P，不用罚参数；连续场本来就满足该约束。
区域阶跃初值在该约束下做几何加权 L2 投影，保持总盘存，投影不保证逐点非负。
外边界只作用于父网格外表面；分域输出沿用 PVD/PVTU/VTU 流程。
当前没有有限界面传质阻力、分配系数或浓度跃变模型；它们需要另行添加模型及双侧积分贡献。
