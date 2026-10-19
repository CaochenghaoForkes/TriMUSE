# TriMUSE

Python 负责 XML 输入、几何与网格生成及流程调度, C++ 基于 MFEM 实现有限元计算, 通过 pybind11 暴露 `_core` 扩展。

## 开发环境构建

在已安装 MFEM、MPI、CMake、Ninja、pybind11 的 Conda 环境中, 从项目根目录执行:

```sh
conda activate trimuse-dev
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$CONDA_PREFIX" -DPython_EXECUTABLE="$CONDA_PREFIX/bin/python"
cmake --build build --parallel 4
python trimuse/main.py
```

Python 运行还需要 NumPy、SciPy、Gmsh 和 Pint。当前入口选择 `cases/flux/case.xml`; 二维恒功率算例为 `cases/source/case.xml`。

入口通过自身位置定位项目与 `build/bindings`, 不依赖启动时的工作目录。子卡路径和输出目录相对于总调度 XML 所在目录解析。

实际输出根目录为总调度卡指定目录下的 `seed_<随机种子>/`, 其中 `mesh/` 保存网格, `thermal/` 保存温度输出。网格固定命名为几何生成器名称, 例如 `stochastic_multilayer_particle.msh`, 始终输出同名 VTK。目标 MSH 文件已存在时直接复用, 并从文件恢复物理组映射; 缺少 VTK 时会补写。更改几何或网格配置后, 如需重新生成, 请移走旧 MSH 或选择新的输出目录; 当前不自动比较配置是否变化。

`trimuse/output.py` 统一管理场输出、输入快照和完成标记。`input/` 保存补全明确默认值后的各张 XML 卡, 长度、角度和时间分别使用 m、rad、s, 模型参数使用其声明的 `si_unit`。可通过其中的 `chief.xml` 重新读取; 不适用或未指定的物理参数不会凭空补值。

`simulation_complete.flag` 内容为 JSON, 记录模拟时长、时间步数、输出帧数, 以及各模块在北京时间的起止时间和耗时。仅在全部时间步与输出成功后写入; 标记存在时跳过求解。修改计算配置后需移除标记或更换输出目录。实际后处理提取与绘图仍待接入。

## 项目移动或重命名后

CMake 构建缓存包含绝对路径, 不能随目录迁移直接复用。请在新目录重新配置再编译:

```sh
cmake --fresh -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$CONDA_PREFIX" -DPython_EXECUTABLE="$CONDA_PREFIX/bin/python"
cmake --build build --parallel 4
```

`--fresh` 需要 CMake 3.24 或更新版本。构建会生成 `build/compile_commands.json`, VS Code 使用该文件获取实际头文件路径。

计算结果写入各算例的输出目录, `build/`、计算结果和本地 ParaView 状态文件不纳入版本控制。
