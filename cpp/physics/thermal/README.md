# Thermal

推荐阅读顺序：

1. `common/thermal_domain_model.hpp`、`conduction/fourier_conduction.hpp`：模型只计算弱式需要的系数。
2. `common/thermal_domain_manager.*`：区域模型分配、依赖登记、系数更新与变化标记。
3. `conduction/`、`capacity/`、`source/`：分别注册 Fourier、rho_cp、volumetric_power。
4. `boundary/`：Dirichlet、Neumann、Robin 分别提供约束值、右端项、矩阵系数。
5. `thermal_field.*`：连续/分域温度的独立试探场、单元编号与整体自由度映射。
6. `interface/`：界面配置、模型注册、双侧点访问与连续约束构建, 具体阅读顺序见下文。
7. `thermal_assembly.*`：积分组装、贡献缓存、约束及线性系统形成。
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

省略 continuity 时仍为 continuous。当前分域求解入口仅接通 temperature，使用固定、协调网格，
各区域采用相同 H1 阶次。积分始终使用原网格的单元映射；子网格负责有限元及自由度，
属性仍按原网格积分点编号读取。

thermal 卡的 interfaces 中，每个内部界面可选择：

```xml
<interface name="interface_1" model="perfect_contact"/>
<interface name="interface_2" model="insulated"/>
```

- 未指定的界面默认 perfect_contact。用精确自由度约束 P 保证温度连续，求解 P^T A P，不使用大罚参数。
- insulated 不连接两侧, 要求分域空间。

界面框架全部位于 `thermal/interface/`：

1. `thermal_interface_model.hpp`：配置和模型基类。模型接收一个网格界面片的 context, 向组装器提供约束。
2. `perfect_contact.*`、`insulated.*`：分别实现连接与绝热, 每个模型独立文件。
3. `thermal_interface_context.*`：引用现有 Interface、ThermalField 和 FemSystem,
   提供双侧几何变换、求积规则, 以及同一界面积分点的实际 x/y/z 和两侧试探温度。
   点值按需读取, 不缓存旧的试探温度；两侧顺序保持 MFEM 的 Elem1/Elem2 顺序。
4. `thermal_interface_assembly.*`：根据父连续空间编号匹配自由度, 汇集连接并形成 P。
5. `thermal_interface_manager.*`：复用 ModelRegistry 注册模型, 按界面物理标识分配模型,
   为未指定的界面使用默认理想接触, 调用模型构建约束。

当前两个模型仅需要固定的拓扑约束, 因而只在初始化时构建一次, 不计算界面积分点数据。
以后增加空间变化热阻等积分型模型时, 可复用 context 的双侧点访问, 增加具体物理求值
和局部积分贡献接口, 并在时间步/迭代更新阶段组装。动态系数、历史损伤与接触状态更新尚未实现。

独立区域初温可不同；理想接触时用几何加权 L2 投影构造连续的初始状态。
外边界只作用于原网格的外部面，不使用子网格自动生成的内部边界标识。
输出仍为同一个 PVD/PVTU/VTU 时间序列，分域场转换到父网格上的不连续输出空间，保留两侧值。
现有点/线采样在恰好落到间断界面的点上仍只返回被定位到的一侧，尚无“指定侧”选项。

## 验证

在项目根目录执行：

```sh
cmake -S . -B build -DTRIMUSE_BUILD_THERMAL_TESTS=ON
cmake --build build --parallel 4
./build/cpp/trimuse_thermal_test /tmp/trimuse-thermal-tests
```

检查连续/分域理想接触等价性、1D/2D/3D 的一二阶及曲边网格、稳态/瞬态非线性、
三种 theta 格式、界面点坐标与双侧温度、混合界面模型、绝热区域独立性、
缓存失效范围及连续空间悬挂节点约束。
测试数据写入指定目录。测试构建默认关闭，可用 `-DTRIMUSE_BUILD_THERMAL_TESTS=OFF` 恢复。
