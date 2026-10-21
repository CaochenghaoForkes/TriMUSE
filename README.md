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

Python 运行还需要 NumPy、SciPy、Gmsh、Pint、VTK、Matplotlib 和 Pillow。通过命令行指定总调度卡:

```sh
python trimuse/main.py cases/transient_real_particle_source_2d/case.xml
python trimuse/main.py -i cases/transient_real_particle_source_2d/case.xml --mode postprocess
```

总调度卡可选配置 `<workflow mode="ask"/>`, 省略时默认 `ask`。命令行 `--mode` 优先于卡内设置:

- `ask`: 空目录直接计算; 已有完整结果时选择换新目录重算、复用结果只后处理或取消; 非空但不完整的目录只能换目录重算或取消。
- `run`: 新计算并后处理, 要求实际输出目录不存在或为空, 不覆盖旧文件。
- `postprocess`: 仅后处理, 要求实际输出目录存在 `simulation_complete.flag`, 不加载 C++ 求解内核。

交互选择新目录时输入的是用户输出根目录, 程序自动追加 `seed_<随机种子>`; 相对路径仍以总调度卡所在目录为基准。直接回车会使用带 `_2`、`_3` 等后缀的新根目录。无交互终端请显式使用 `run` 或 `postprocess`。

入口通过自身位置定位项目与 `build/bindings`, 不依赖启动时的工作目录。子卡路径和输出目录相对于总调度 XML 所在目录解析。

实际输出根目录为总调度卡指定目录下的 `seed_<随机种子>/`, 其中 `mesh/` 保存网格, `thermal/` 保存温度输出。网格固定命名为几何生成器名称, 例如 `stochastic_multilayer_particle.msh`, 始终输出同名 VTK。目标 MSH 文件已存在时直接复用, 并从文件恢复物理组映射; 缺少 VTK 时会补写。更改几何或网格配置后, 如需重新生成, 请移走旧 MSH 或选择新的输出目录; 当前不自动比较配置是否变化。

`trimuse/output.py` 统一管理场输出、输入快照和完成标记。`input/` 保存补全明确默认值后的各张 XML 卡, 长度、角度和时间分别使用 m、rad、s, 模型参数使用其声明的 `si_unit`。可通过其中的 `chief.xml` 重新读取; 不适用或未指定的物理参数不会凭空补值。

`simulation_complete.flag` 内容为 JSON, 记录网格统计、模拟时长、时间步数、输出帧数、北京时间起止时间和总耗时。仅在全部时间步与输出成功后写入, 作为完整结果的标记。修改计算配置后请通过 `ask` 选择新目录重算, 或修改输出目录再使用 `run`; 仅删除完成标记不会把非空目录视为空目录。

## 项目移动或重命名后

CMake 构建缓存包含绝对路径, 不能随目录迁移直接复用。请在新目录重新配置再编译:

```sh
cmake --fresh -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$CONDA_PREFIX" -DPython_EXECUTABLE="$CONDA_PREFIX/bin/python"
cmake --build build --parallel 4
```

`--fresh` 需要 CMake 3.24 或更新版本。构建会生成 `build/compile_commands.json`, VS Code 使用该文件获取实际头文件路径。

计算结果写入各算例的输出目录, `build/`、计算结果和本地 ParaView 状态文件不纳入版本控制。
