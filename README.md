# TriMUSE

Python 负责 XML 输入、几何与网格生成及流程调度, C++ 基于 MFEM 实现有限元计算, 通过 pybind11 暴露 `_core` 扩展。

## 开发环境构建

在已安装 MFEM、MPI、Boost（仅头文件）、CMake、Ninja、pybind11 的 Conda 环境中, 从项目根目录执行:

```sh
conda activate trimuse-dev
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$CONDA_PREFIX" -DPython_EXECUTABLE="$CONDA_PREFIX/bin/python"
cmake --build build --parallel 4
python trimuse/main.py
```

Python 运行还需要 NumPy、SciPy、Gmsh、Pint、VTK、Matplotlib 和 Pillow。

在 `trimuse/main.py` 文件末尾直接指定总调度卡，然后运行该文件：

```python
if __name__ == "__main__":
    main(project_directory / "cases/transient_heat_transient_cs137_2d_2/case.xml")
```

也可以在 Python 中导入并调用：

```python
from trimuse.main import main

main("cases/transient_heat_transient_cs137_2d_2/case.xml")
```

每次运行固定执行“网格生成 → 模拟 → 后处理”。传入 `main()` 的相对路径以当前工作目录为基准；总调度卡内的子卡和输出目录相对于总调度卡所在目录解析。

实际输出目录为 `<用户目录>/seed_<随机种子>/`。如果该目录已经存在，即使为空，也自动改用 `<用户目录>_2/seed_<随机种子>/`、`<用户目录>_3/seed_<随机种子>/` 等新目录；原结果保留。本次实际目录会同步用于场输出、输入快照、报告和后处理。

网格保存在实际输出目录的 `mesh/` 下，按几何生成器名称命名，例如 `stochastic_multilayer_particle.msh`，同时输出同名 VTK 及输入中要求的其他格式。温度和浓度场分别保存在 `thermal/` 和 `radioactivity/` 下。

`trimuse/output.py` 统一管理场输出、输入快照和完成标记。`input/` 保存补全明确默认值后的各张 XML 卡, 长度、角度和时间分别使用 m、rad、s, 模型参数使用其声明的 `si_unit`。可通过其中的 `chief.xml` 重新读取; 不适用或未指定的物理参数不会凭空补值。

`simulation_report.txt` 按求解顺序记录各场的 CG、Picard 统计和耗时, 结束时追加网格统计、模拟时长、时间步数、输出帧数、北京时间起止时间和总耗时。全部计算和输出成功后, 最后一行写入 `simulation_completed`, 不再生成独立 flag 文件。完成标记用于记录该次模拟已结束；后续运行自动选择新目录。

## 项目移动或重命名后

CMake 构建缓存包含绝对路径, 不能随目录迁移直接复用。请在新目录重新配置再编译:

```sh
cmake --fresh -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$CONDA_PREFIX" -DPython_EXECUTABLE="$CONDA_PREFIX/bin/python"
cmake --build build --parallel 4
```

`--fresh` 需要 CMake 3.24 或更新版本。构建会生成 `build/compile_commands.json`, VS Code 使用该文件获取实际头文件路径。

计算结果写入各算例的输出目录, `build/`、计算结果和本地 ParaView 状态文件不纳入版本控制。
