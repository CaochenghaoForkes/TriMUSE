"""总调度 XML 输入卡的配置对象与读取器"""

from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from xml.etree import ElementTree as ET
from .xml_utils import XmlUtils

@dataclass
class CaseConfig:
    """一个 TriMUSE 算例的总调度配置"""

    @dataclass
    class InputFiles:
        """算例引用的各张输入卡路径"""

        geometry_mesh: str | None = None
        thermal: str | None = None
        diffusion: str | None = None
        mechanics: str | None = None
        materials: str | None = None
        coupling: str | None = None
        numerics: str | None = None

    @dataclass
    class Execution:
        """本次运行的终止阶段"""

        target: str  # [mesh,solve,postprocess]

    @dataclass
    class Output:
        """整个算例的输出根目录"""

        directory: str

    schema_version: str  # [0.1]
    name: str
    random_seed: int
    output: Output
    description: str | None = None
    input_files: InputFiles = field(default_factory=InputFiles)
    execution: Execution | None = None

class CaseConfigTemplate:
    """维护完整节点模板并提供文件写出入口"""

    CONTENT = '''<?xml version="1.0" encoding="UTF-8"?>
<!-- 填写占位符后使用, 枚举值只选一项, 不使用的可选节点请删除 -->
<trimuse_case schema_version="[0.1]" name="[str]">
    <!-- description 可选 -->
    <description>[str]</description>
    <random seed="[int]"/>
    <output directory="[str]"/>
    <input_files>
        <!-- 当前运行阶段需要 geometry_mesh, 其他输入卡读取尚未接入 -->
        <!-- geometry_mesh 的 file 留空时生成模板, 其余卡的模板尚未实现 -->
        <geometry_mesh file="[str]"/>
        <thermal file="[str]"/>
        <diffusion file="[str]"/>
        <mechanics file="[str]"/>
        <materials file="[str]"/>
        <coupling file="[str]"/>
        <numerics file="[str]"/>
    </input_files>
    <!-- 当前已实现网格阶段, solve 和 postprocess 为预留选项 -->
    <execution target="[mesh][solve][postprocess]"/>
</trimuse_case>
'''

    @classmethod
    def write(cls, path: str | Path = "case_empty.xml") -> Path:
        """生成待填写输入卡, 默认写入当前工作目录"""
        return XmlUtils.write_template(cls.CONTENT, path)

class CaseConfigLoader:
    """总调度 XML 输入卡读取器"""

    @classmethod
    def load(cls, path: str | Path | None = None) -> CaseConfig | Path:
        """路径为空时生成模板, 否则读取配置"""
        if path is None or isinstance(path, str) and not path.strip():
            return CaseConfigTemplate.write()
        card_path = Path(path).expanduser().resolve()
        context = str(card_path)
        root = XmlUtils.read_xml(card_path, root_tag="trimuse_case")
        XmlUtils.check_attributes(root, ("schema_version", "name"), context=context)
        XmlUtils.check_children(root,("description", "random", "output", "input_files", "execution"),context=context)

        schema_version = XmlUtils.read_string(root,"schema_version",required=True,context=context)
        name = XmlUtils.read_string(root, "name", required=True, context=context)
        description_element = XmlUtils.read_child(root,"description",required=False,context=context)
        random_element = XmlUtils.read_child(root, "random", context=context)
        output_element = XmlUtils.read_child(root, "output", context=context)
        input_files_element = XmlUtils.read_child(root, "input_files", context=context)
        execution_element = XmlUtils.read_child(root, "execution", context=context)

        description = None
        if description_element is not None:
            description = cls.read_description(description_element, context=context)
        assert schema_version is not None
        assert name is not None
        assert random_element is not None
        assert output_element is not None
        assert input_files_element is not None
        assert execution_element is not None
        return CaseConfig(
            schema_version=schema_version,
            name=name,
            random_seed=cls.read_random(random_element, context=context),
            output=cls.read_output(output_element, context=context),
            description=description,
            input_files=cls.read_input_files(input_files_element, context=context),
            execution=cls.read_execution(execution_element, context=context),
        )

    @staticmethod
    def read_random(element: ET.Element, *, context: str | None = None) -> int:
        """读取整个算例共用的随机种子"""
        XmlUtils.check_attributes(element, ("seed",), context=context)
        XmlUtils.check_children(element, (), context=context)
        seed = XmlUtils.read_int(element, "seed", required=True, context=context)
        assert seed is not None
        if seed < 0:
            raise ValueError(f"{XmlUtils._location(element, context)}: seed 必须是非负整数")
        return seed

    @staticmethod
    def read_output(element: ET.Element, *, context: str | None = None) -> CaseConfig.Output:
        """读取整个算例的输出根目录"""
        XmlUtils.check_attributes(element, ("directory",), context=context)
        XmlUtils.check_children(element, (), context=context)
        directory = XmlUtils.read_string(element, "directory", required=True, context=context)
        assert directory is not None
        return CaseConfig.Output(directory=directory)

    @staticmethod
    def read_description(
        element: ET.Element,
        *,
        context: str | None = None,
    ) -> str:
        """读取算例文字说明"""
        XmlUtils.check_attributes(element, (), context=context)
        description = XmlUtils.read_text(element, required=True, context=context)
        assert description is not None
        return description

    @staticmethod
    def read_input_files(
        element: ET.Element,
        *,
        context: str | None = None,
    ) -> CaseConfig.InputFiles:
        """读取各输入卡的相对或绝对路径"""
        input_names = (
            "geometry_mesh",
            "thermal",
            "diffusion",
            "mechanics",
            "materials",
            "coupling",
            "numerics",
        )
        XmlUtils.check_attributes(element, (), context=context)
        XmlUtils.check_children(element, input_names, context=context)
        input_files = CaseConfig.InputFiles()
        for name in input_names:
            input_element = XmlUtils.read_child(element,name,required=False,context=context)
            if input_element is not None:
                setattr(input_files,name,CaseConfigLoader.read_input_file(input_element, context=context))
        return input_files

    @staticmethod
    def read_input_file(
        element: ET.Element,
        *,
        context: str | None = None,
    ) -> str:
        """读取子卡路径, 空值或缺省 file 返回空字符串以请求模板"""
        XmlUtils.check_attributes(element, ("file",), context=context)
        XmlUtils.check_children(element, (), context=context)
        return element.get("file", "").strip()

    @staticmethod
    def read_execution(
        element: ET.Element,
        *,
        context: str | None = None,
    ) -> CaseConfig.Execution:
        """读取本次运行的终止阶段"""
        XmlUtils.check_attributes(element, ("target",), context=context)
        XmlUtils.check_children(element, (), context=context)
        target = XmlUtils.read_string(element, "target", required=True, context=context)
        assert target is not None
        return CaseConfig.Execution(target=target)

def load_case_config(path: str | Path | None = None) -> CaseConfig | Path:
    """兼容函数式入口的 case XML 输入卡读取函数"""
    return CaseConfigLoader.load(path)

def write_case_template(path: str | Path = "case_empty.xml") -> Path:
    """生成完整 XML 模板并返回路径, 不覆盖已有文件"""
    return CaseConfigTemplate.write(path)
