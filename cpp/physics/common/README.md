# Physics common

这里保存物理模块共用的场、约束、界面几何和组装数据操作，不注册具体物理模型。
各模块通过持有对象或调用函数复用它们。依赖指向 fem/MFEM，公共代码不引用 thermal 或 radioactivity。

建议按以下顺序阅读：

1. `field/trial_field.*`：统一访问连续场和分域场；保存独立试探值，提供单元/面自由度映射、`load()`、`accept()` 和 `FieldView`。支持标量与多分量 H1 场。
2. `interface/interface_context.*`：一个原网格界面片的几何入口。`point()` 返回实际坐标与两侧参考单元坐标，具体模块再用 `FieldView` 取得所需场值。
3. `constraint/dof_constraints.*`：按分量连接两侧对应的 H1 自由度，生成 P，并完成方程约化、指定值消元与完整向量恢复。
4. `assembly/scalar_integrator.*`：标量 H1 场的扩散、容量、体积源与外边界积分，thermal 和 radioactivity 共用；矢量力学需要自己的积分器。
5. `assembly/assembly_cache.hpp`、`assembly_utils.*`：按区域/边界保存只读贡献，求和生成新矩阵或向量。物理模块负责判断是否需要重新组装。

## 场与约束的调用顺序

```cpp
TrialField trial(fem, field); // field 可以是 FieldData 或 RegionFieldData
DofConstraints constraints(fem, trial);
for (const auto& interface : fem.mesh_data().interfaces())
{
    // 只对物理模型要求场值连续的界面调用；未连接的分域自由度保持独立。
    constraints.connect(interface);
}
constraints.finalize(); // 连接完成后调用一次；此后不再 connect

// A、b 使用完整自由度编号，essential 使用独立自由度编号。
constraints.form_system(A, b, essential, system, load, solution);
// 由物理模块调用求解器，得到 solution。
trial.set_values(constraints.recover(solution));
trial.accept(); // 接受求解结果后才写回正式场
```

`finalize()` 后释放配对用的父连续空间和自由度分组，只保留 P 与试探场引用。
试探场必须在约束对象使用期间保持存活且位置稳定；网格与空间布局在求解期间保持不变。
区域场沿用各区域同阶的协调 H1 网格；连续场可使用 MFEM 的悬挂节点约束。

`x_full = P u`，约化方程为 `P^T A P u = P^T b`。
分域相等约束的 `reduced_values()` 对组内场值取平均，作为初始猜测；方程右端只相加，不取平均。
连续非协调场的初始猜测由 MFEM 的限制矩阵取得。
`recover()` 返回完整向量，不修改正式场或试探场。

相等约束支持标量连续和向量各分量连续。接触、滑移、非匹配界面及一般多点约束不在当前接口范围内。
指定值的物理含义、边界选择与分量选择由各模块处理，公共类只接收独立编号和数值。
热学的初始 L2 投影、热阻积分、模型注册、时间推进与非线性调度仍在 thermal 中。

## 缓存约定

`AssemblyCache<Value>::parts` 保存各部分贡献，`sum` 保存汇总结果，均采用完整场的整体编号。
贡献变化时替换共享指针；`sum_matrices()` / `sum_vectors()` 创建新对象，不改写旧贡献或旧状态快照。
Dirichlet 消元只操作 `form_system()` 输出的工作副本。是否重组以及何时更新总和，由具体物理模块决定。
