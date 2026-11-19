# 二维 VTU 场对比

`compare_vtu.py` 生成 TriMUSE、COMSOL、TriMUSE−COMSOL 三联图，每个字段输出 PNG 和 PDF。支持二维平面或轴对称截面的标量场，输入坐标使用米，绘图坐标展示为微米；不用于直接绘制三维体网格。

依赖：NumPy、Matplotlib、VTK。推荐在 trimuse-dev 环境运行。字体使用 Times New Roman。

在项目根目录执行：

```bash
python doc/compare_vtu.py \
  --trimuse-dir cases/region_heat_resistance/output/seed_20261111/mechanics \
  --comsol cases/region_heat_resistance/comsol.vtu \
  --field max_principal_stress 第一主应力 sigma1 Pa \
  --field max_shear_stress Data tau_max Pa \
  --output cases/region_heat_resistance/comparison
```

每个 `--field` 的四个参数依次为 TriMUSE 字段名、COMSOL 字段名、图例标签、单位，可以重复指定。`Data` 只是当前 COMSOL 文件中的数组名称，须确认它对应最大剪应力，脚本无法从这个名称判断原始表达式。输入单位指定为 Pa 时，绘图会将场值与差值共同换算为 MPa；其他单位按输入数值展示。

默认选取所有字段共有的最后一个 TriMUSE 输出周期，可通过 `--cycle 1` 指定；COMSOL 文件应提前导出相同物理状态。

前两联共用色标；差值采用独立的正负对称色标。两份结果使用单元平面着色，不额外平滑。COMSOL 场通过 VTK 单元插值采样到 TriMUSE 三角形中心，再与该三角形中心的 TriMUSE 值相减。不在 COMSOL 网格内的采样点被遮罩，不作外推。

轮廓由 TriMUSE 的 attribute 材料编号提取，叠加到三联图上。COMSOL 导出文件应保留界面两侧的数据跳跃；已经在导出时平均的数据无法由本脚本恢复。当前差值是输出场的采样对比，不等同于求积点原始应力误差或网格收敛性分析。

算例目录中的 `compare_comsol.py` 保留了当前字段映射、seed 和路径默认值，直接运行该入口也会调用同一套通用实现。

图形采用 Times New Roman、等比例坐标、(a)–(c) 分图编号。前两联为 jet 共用线性色标，第三联为蓝–白–红的零中心差值色标。两个横向色标放在图下方，材料轮廓使用细深灰线；灰色区域表示网格覆盖以外或无有效参考采样的位置。算例入口只保留参数和字段映射，通用读取、采样、绘图及周期选择均位于 compare_vtu.py。
