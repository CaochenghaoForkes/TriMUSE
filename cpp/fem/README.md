# 全域场与分区域场

原来的 `add_field()` 保持全域 H1 连续; 新的 `add_region_field()` 在每个区域建立独立 H1 空间。
区域内部共享自由度, 不同区域的界面自由度不合并。两类场可以共用同一原网格:

```cpp
FemSystem fem(mesh_path, 5, 5);
fem.add_field("temperature", 2);
fem.add_region_field("concentration", 2);

auto& concentration = fem.region_field("concentration");
concentration.field_data(3).assign(1.0);
concentration.field_data(9).assign(0.0);
auto& space = concentration.finite_element_space(3).mfem_space();
```

`MeshData::region_mesh(attribute)` 按需创建并缓存区域子网格, 多个区域场共用网格而不共用自由度。
二维、三维使用 MFEM SubMesh; 一维单独提取线单元, 保留高阶几何节点及原边界编号。
子网格会为未标记的切面生成边界编号; 物理界面的原始标识应以 `interfaces()` 为准。
`RegionMeshData` 提供原网格与区域网格的单元/面编号映射; 未属于该区域的原单元或面映射为 -1。
`RegionFieldData::dof_offsets()` 和 `true_dof_offsets()` 按 `MeshData::region_attributes()` 顺序给出分块偏移量。
`get_values()` / `set_values()` 在区域 GridFunction 与完整分块向量之间复制数值, 不涉及真实自由度约束变换。

## 双侧界面

`MeshData::interfaces()` 自动收集相邻单元区域编号不同的内部面, 不包括同区域内部面和外边界。
每条记录保存原网格面编号、原输入边界编号以及两侧区域/单元编号。
未在输入网格中标记的内部界面仍能识别, 其 `boundary_attribute` 为 0。
两侧顺序与 MFEM 的 `Elem1/Loc1`、`Elem2/Loc2` 一致, 不是按区域编号排序。

```cpp
const auto& interface = fem.mesh_data().interfaces()[interface_index];
auto& transformation = fem.mesh_data().interface_transformations(interface_index);
const auto& side = interface.sides[0];

mfem::IntegrationPoint element_point;
transformation.Loc1.Transform(face_point, element_point);
const int element = concentration.local_element(side.element);
double value = concentration.field_data(side.region_attribute)
                   .mfem_grid_function().GetValue(element, element_point);
```

第二侧使用 `Loc2`。同一个参考面积分点通过两侧映射后对应同一物理位置, 不应按两侧自由度列表下标直接配对。
`interface_dofs()` 返回单侧迹自由度的分块整体编号; `element_dofs()` 返回域单元的分块整体编号。
这些编号保留 MFEM 的有符号编号约定, 尚未消去本质边界条件。
`interface_transformations()` 返回 MFEM 共用的临时对象, 不应跨下一次面变换调用保存它或其中的指针。

## 范围

- 当前面向固定、协调的共形网格; 不能在建立空间或缓存后独立细化/移动原网格或子网格。
- 初始已重合的材料界面由网格拓扑识别; 不搜索间隙两侧的非重合面, 也不处理非匹配接触。
- 此层只提供独立自由度和双侧几何关系, 不添加热阻、传质、黏结、压力或接触等物理项。
- 现有物理模块、Python 绑定和输出流程仍走原来的全域场接口, 不会自动改用区域场。
- 后续区域物理积分器读取现有积分点属性时, 必须先将区域单元号映射回原单元号。
- 一维球对称、二维轴对称积分应继续使用原 `FemSystem::geometric_weight()` 的全局径向原点,
  不能把每个子网格的最小坐标当作新的球心或旋转轴。

## 检查

手动构建测试目标, 不影响日常构建:

```sh
cmake --build build --target trimuse_fem_region_test --parallel 4
task_test_dir=$(mktemp -d)
build/cpp/trimuse_fem_region_test "$task_test_dir"
```

覆盖一维/二维/三维、线/三角形/四边形/四面体/六面体、一阶/二阶/三阶、标量/三分量场、
有/无界面边界标识及曲边映射。检查区域内部连接、两侧独立性、父子编号、双侧非恒定迹值和分块向量。
