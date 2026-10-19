from pathlib import Path
from ..xml_utils import XmlUtils

class ChiefCardTemplate:
    """总调度卡模板, 不包含已删除的版本号和正文说明"""

    CONTENT = '''<?xml version="1.0" encoding="UTF-8"?>
<!-- 填写占位符后使用, 可选值只选一项 -->
<trimuse_chief name="[str]">
    <!-- 整个算例共用的随机种子 -->
    <random seed="[int]"/>
    <!-- 输出根目录, 相对路径以本卡所在目录为基准 -->
    <output directory="[str]"/>
    <input_files>
        <!-- 子卡相对路径以本卡所在目录为基准, file 留空时生成对应模板并退出 -->
        <geometry_mesh file="[str]"/>
        <material file="[str]"/>
        <properties file="[str]"/>
        <thermal file="[str]"/>
        <simulation file="[str]"/>
    </input_files>
    <!-- 当前实现 mesh, solve 和 postprocess 为预留阶段 -->
    <execution target="[mesh][solve][postprocess]"/>
</trimuse_chief>
'''

    @classmethod
    def write(cls, path: str | Path = "chief_empty.xml") -> Path:
        """写出待填写模板, 由外部 loader 调用"""
        return XmlUtils.write_template(cls.CONTENT, path)
