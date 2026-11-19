# 开发准则

- 本项目由用户和 Codex 两人共同开发，以直观、简洁、易读为优先
- 采用讨论确定的唯一标准，直接淘汰旧版本、旧格式和旧接口
- 修改时同步更新调用处、输入卡和模板，删除过时实现
- 程序内部按约定直接使用数据，发现错误就修改根源
- 避免防御性判断、兜底分支、静默回退和额外的 throw
- 按当前实际需求实现，避免为假想使用者预留接口或添加抽象
- config 的数据结构与 XML 输入卡逐层对应
- 发生设计冲突时，与用户讨论并确定唯一标准

## C++ 代码风格

- 使用 4 个空格缩进，不使用制表符，代码块的左右花括号各自独占一行，空函数体也显式保留花括号
- 函数签名及全部参数必须写在同一行，包括头文件声明、自由函数和 `.cpp` 中的定义，不因行长而折行
- 头文件中，简单行内函数的完整实现可以放在一行；需要多行实现的普通函数只保留单行声明，将实现移至对应 `.cpp`
- 构造函数的实现必须放在 `.cpp`，即使函数体为空也要显式写出；模板等必须在头文件中定义的实现以及编译器生成的 default/delete 声明除外
- public 中相邻函数声明或行内函数之间不留空行
- private 中先放结构体、枚举、类型别名等类型定义，再空一行放私有方法声明，再空一行放成员变量；同组函数之间不留空行，没有对应分组时不额外留空行
- 整理声明分组时保留类型之间的依赖顺序和成员变量原有的构造依赖顺序，不为排版改变初始化语义
- 构造函数的初始化列表中，冒号单独一行，每个初始化项各占一行，均缩进 4 个空格；同一个成员的聚合初始化内容尽量合为一行
- 每个逻辑判断表达式写在一行，不在 `&&` 或 `||` 处折行；return 中的组合条件同样写成一行
- `.cpp` 中两个函数实现之间空一行，namespace 的左花括号与首个定义之间空一行，两个 namespace 之间也空一行
- 连续空行最多一行，注释行不算空行
- 针对某一行代码的注释放在该行末尾；内容很长或说明多个语句、整体原理的注释可以写在代码上方，注释不加句号
- 修改格式后检查全部相关声明、定义与引用，涉及实现迁移时同步维护构建源文件列表，并通过编译与适当的数值结果对比验证

构造函数格式示例：

```cpp
MechanicsAssembly::MechanicsAssembly(FemSystem& fem, TrialField& trial_field, const MechanicsRegionConfigs& regions, const MechanicsBoundaryConfigs& boundaries, const MechanicsInterfaceConfigs& interfaces) // 初始化管理器和界面连接
    :
    fem_(fem),
    trial_field_(trial_field),
    integrator_(fem, trial_field),
    constraints_(fem, trial_field),
    stiffness_manager_(fem, regions.at("stiffness")),
    thermal_expansion_manager_(fem, regions.at("thermal_expansion")),
    source_manager_(fem, regions.at("source")),
    boundary_manager_(boundaries.at("mechanics_boundary")),
    interface_contact_manager_(fem, constraints_, interfaces.at("interface_contact"))
{
}
```

## C++ 变量命名

- 管理器变量使用 `xxx_manager_`
- 矩阵缓存使用 `xxx_matrix_cache_`，载荷向量缓存使用 `xxx_load_cache_`
- 上次汇总使用的缓存指针使用 `last_xxx_matrix_cache_sum_` 或 `last_xxx_load_cache_sum_`
- 各物理问题的 State 统一使用 `mass`、`matrix`、`load`；准静态力学不添加无用的 mass
- Problem 中完整自由度方程使用 `matrix`、`load`，约化并施加约束后的系统使用 `system`、`system_load`、`solution`，旧时刻右端贡献使用 `old_load`
- Assembly 内部的管理器、缓存和 refresh 方法按具体物理项命名，保留 conduction、capacity、diffusion、decay、stiffness 等物理含义

## 属性模型约定

- 时间累积模型的初值构造参数统一命名为 `initial_value`
