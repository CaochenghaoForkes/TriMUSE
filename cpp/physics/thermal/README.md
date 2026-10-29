# Thermal

推荐阅读顺序：

1. `common/thermal_domain_model.hpp`、`conduction/fourier_conduction.hpp`：模型只计算弱式需要的系数。
2. `common/thermal_domain_manager.*`：区域模型分配、依赖登记、系数更新与变化标记。
3. `conduction/`、`capacity/`、`source/`：分别注册 Fourier、rho_cp、volumetric_power。
4. `boundary/`：Dirichlet、Neumann、Robin 分别提供约束值、右端项、矩阵系数。
5. `../common/field/trial_field.*`：连续/分域场的独立试探场、单元编号与整体自由度映射；热问题直接使用 `TrialField`。
6. `interface/`：界面配置、模型注册、双侧点访问与连续约束构建, 具体阅读顺序见下文。
7. `thermal_assembly.*`：贡献更新与热边界处理；域内与外边界积分使用公共 `ScalarIntegrator`；缓存和求和使用 `../common/assembly/`，约束与代数系统转换使用 `../common/constraint/`。
8. `thermal_problem.*`：初始状态、theta 时间离散、Picard 迭代与收敛提交。

## 模型与管理器

具体模型实现 `evaluate`，管理器通过 `ModelRegistry` 注册模型及其属性依赖。
添加同类物理模型时，在对应目录实现模型并注册。导热与热容目前各有一个模型，默认应用于所有区域；
其管理器也接受按区域配置的模型列表。源、外边界及界面配置由 thermal 输入卡提供。

域内管理器共用 `update`、`mark_coupled`、`coupled`、`required_no_change`、`assembled`。
`update` 比较实际进入弱式的系数（如 rho*cp），发生变化后保持 dirty，直到组装完成。
`required_no_change(region)` 表示该区域的系数与上次组装一致。
边界分别记录 Matrix、Load、Essential 的变化，因此环境温度变化只影响 Robin 右端项。

## 缓存与时间推进

热容、导热、体积源、边界矩阵和边界右端项分别按区域/外边界缓存。
未变化的部分跨 Picard 迭代、跨时间步复用；局部变化只重组对应区域或边界。
各部分之和仅在部分发生变化时更新。改变 dt 或 theta 仍需组合最终线性系统，但不必重新做单元积分。

缓存保存未消元矩阵。Dirichlet 消元作用于工作副本，因此改变表面温度不会污染缓存。
旧时刻的 State 共享只读矩阵/向量；更新时替换指针，保证 theta 法使用的旧状态不变。
时间离散保持：

```
M_theta = (1-theta) M_n + theta M_{n+1}
A = M_theta/dt + theta K_{n+1}
b = M_theta*T_n/dt + theta F_{n+1} + (1-theta)(F_n-K_n*T_n)
```

theta=1 不读取旧属性快照；theta=0 只使用旧时刻的域内及自然边界贡献，但约束新时刻的指定温度。
收敛后更新温度及其耦合材料属性；属性历史快照仍由外部时间调度统一提交。
`assembly_counts()` 返回各类贡献的累计组装次数，按区域/边界计数；
每次 solve 的增量同时写入求解信息，供核查缓存是否生效。

## 分域温度与界面

simulation 卡中设置：

```xml
<field name="temperature">
    <fem_space family="H1" order="2" continuity="region"/>
    <!-- 其余求解器配置沿用原格式 -->
</field>
```

省略 continuity 时仍为 continuous。temperature 与 concentration 均已接通分域求解入口；分域场使用固定、协调网格，
各区域采用相同 H1 阶次。积分始终使用原网格的单元映射；子网格负责有限元及自由度，
属性仍按原网格积分点编号读取。

thermal 卡的 interfaces 中，每个内部界面可选择：

```xml
<interface name="interface_1" model="perfect_contact"/>
<interface name="interface_2" model="uniform_from_constant_resistance">
    <parameter name="constant_resistance" value="1e-4" unit="[m]^2[K]/[W]" si_unit="[m]^2[K]/[W]"/>
</interface>
```

- 未指定的界面默认 perfect_contact。用精确自由度约束 P 保证温度连续，求解 P^T A P，不使用大罚参数。
- uniform_from_constant_resistance 要求分域空间, 参数 constant_resistance 是单位面积热阻 R,
  单位 m²·K/W, 必须为有限正数。整个界面上 q=(T_a-T_b)/R, 两侧温度可以不同。
  R=0 的精确连续条件用 perfect_contact；独立的 insulated 模型已删除。

热界面模型位于 `thermal/interface/`，几何访问与相等约束复用 `physics/common/`：

1. `thermal_interface_model.hpp`：配置和模型基类。模型接收一个网格界面片的 context, 向组装器提供约束或传热贡献。
2. `perfect_contact.*`、`uniform_from_constant_resistance.*`：分别实现理想接触与恒定热阻, 每个模型独立文件。
3. `thermal_interface_context.*`：持有公共 `InterfaceContext` 并引用 `TrialField`，
   几何变换、求积规则、实际 x/y/z 和两侧参考坐标由公共 context 提供，热 context 再读取两侧试探温度。
   点值按需读取, 不缓存旧的试探温度；两侧顺序保持 MFEM 的 Elem1/Elem2 顺序。
4. `thermal_interface_assembly.*`：通过公共 `DofConstraints` 汇集自由度连接形成 P，汇集传热贡献形成界面矩阵。
   `thermal_interface_integrator.*`：逐面积分 h[N_a;-N_b][N_a;-N_b]^T,
   形成两侧对角和交叉矩阵块, 并使用 1D/2D 的几何权重。
5. `thermal_interface_manager.*`：复用 ModelRegistry 注册模型, 按界面物理标识分配模型,
   为未指定的界面使用默认理想接触, 保存初始化得到的约束和界面矩阵。

公共组件的职责和调用顺序见 `../common/README.md`。这里的 `thermal/common/` 仍只管理热学域模型，
温度必须为标量的检查保留在 `ThermalProblem` 入口；物理模型、时间推进和缓存失效判断仍由 thermal 负责。

当前两个模型的约束或系数均固定, 因而只在初始化时构建一次, 后续时间步和迭代复用。
恒定热阻只增加导热矩阵, 不增加热容或外部热源, 双侧传热贡献大小相等、符号相反。
积分器接收 mfem::Coefficient, 可在各积分点求值。以后增加空间变化热阻时可提供相应系数对象,
并复用 context 的坐标/双侧取值及现有积分器。随时间或场值变化的系数还需接入更新与缓存失效；
历史损伤与接触状态更新尚未实现。

独立区域初温可不同；理想接触时用几何加权 L2 投影构造连续的初始状态。
外边界只作用于原网格的外部面，不使用子网格自动生成的内部边界标识。
输出仍为同一个 PVD/PVTU/VTU 时间序列，分域场转换到父网格上的不连续输出空间，保留两侧值。
现有点/线采样在恰好落到间断界面的点上仍只返回被定位到的一侧，尚无“指定侧”选项。

## 验证

在项目根目录执行：

```sh
cmake --build build --parallel 4
```

检查连续/分域理想接触等价性、1D/2D/3D 的一二阶及曲边网格、稳态/瞬态非线性、
三种 theta 格式、界面点坐标与双侧温度、混合界面模型、恒定热阻的温度跳跃与热量守恒、
缓存失效范围及连续空间悬挂节点约束。
