
# 程序介绍

TRISO 颗粒多物理耦合燃料性能分析平台: TriMUSE (TRISO Multiphysics Simulation Environment) 

```TriMUSE
TriMUSE/
├── trimuse/          # Python 输入、几何网格、流程调度和后处理
├── cpp/              # C++ 有限元计算内核
├── bindings/         # Python 与 C++ 的接口
├── cases/            # 算例输入文件、对比脚本及计算结果
├── doc/              # 程序指南和说明文档
├── build/            # CMake 生成的构建目录及编译产物
├── CMakeLists.txt    # 项目构建入口
└── README.md         # 项目说明、环境配置和运行方法
```
# trimuse (Python 层)

```trimuse
trimuse/
├── geometry/       # 几何构建、区域与边界划分、网格生成与优化
├── input/          # XML 输入读取、模板生成、单位换算与配置检查
├── postprocess/    # 结果读取、点线截面采样、云图与动画
├── __init__.py     # Python 包初始化
├── coupling.py     # 分阶段时间推进、属性更新与各物理场求解调度
├── fem.py          # 配置求积规则、有限元空间与连续／分区域场
├── main.py         # 程序入口，组织完整运行流程
├── output.py       # 输出目录、输入快照、运行报告与结果保存管理
├── problem.py      # 整理热与放射性问题配置，创建并调用 C++ 问题对象
├── properties.py   # 配置属性模型、材料分配及属性更新调用
└── solver.py       # 配置线性求解器、预条件器与非线性迭代参数
```

## input (输入模块)

```input
input/
├── chief/           # 总调度卡：子卡路径、随机种子、输出目录与运行方式
├── geometry_mesh/   # 几何与网格卡：颗粒结构、区域边界、网格尺寸与生成配置
├── material/        # 材料卡：材料属性模型及材料与区域的对应关系
├── postprocess/     # 后处理卡：输出场、采样位置与绘图配置
├── properties/      # 属性卡：外部量、派生量及其模型参数与作用范围
├── radioactivity/   # 放射性卡：核素、初始盘存、源、边界与界面条件
├── simulation/      # 模拟卡：问题类型、时间推进、有限元空间、求解器与求解顺序
├── thermal/         # 热问题卡：初始温度、热源、外边界与内部界面条件
├── __init__.py      # 输入包初始化，导出统一读取入口
├── examine.py       # 各输入卡之间的一致性、引用关系及必要依赖检查
├── units.py         # 单位解析与换算
├── xml_loader.py    # 统一加载、检查并汇总各张输入卡
└── xml_utils.py     # XML 节点、属性和模型参数读取，以及模板文件复制
```

### xml_utils.py

```Python 
class XmlUtils:
	@staticmethod
	def _location(element: ET.Element | None, context: str | None = None) -> str:
	
	@staticmethod
	def write_template(template: str | Path, path: str | Path) -> Path:
	
	@staticmethod
	def get_root(path: str | Path, root_tag: str | None = None) -> ET.Element:
	
	@staticmethod
	def read_attribute(
	
	@staticmethod
	def read_model(element: ET.Element, *, context: str | None = None) -> dict[str, str | dict[str, float | list[float]]]:
	
	@staticmethod
	def read_child(
	
	@staticmethod
	def read_children(
	
	@staticmethod
	def check_attributes(
	
	
```
# cpp (C++ 层)

# bindings (接口)

# 程序使用



