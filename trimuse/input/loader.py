"""TriMUSE 输入卡的总加载器"""

from __future__ import annotations
from dataclasses import dataclass
from pathlib import Path
from .case_config import CaseConfig, CaseConfigLoader
from .geometry_mesh_config import GeometryMeshCard, GeometryMeshCardLoader, GeometryMeshCardTemplate
from .geometry_mesh_examine import GeometryMeshExamine
from .xml_utils import XmlUtils

@dataclass
class LoadedCase:
    """已读取并通过当前阶段检查的算例输入"""

    case_path: Path
    case_config: CaseConfig
    geometry_mesh_path: Path
    geometry_mesh_config: GeometryMeshCard

class CaseLoader:
    """从 case XML 递进加载其引用的输入卡"""

    @classmethod
    def load(cls, path: str | Path | None = None) -> LoadedCase | Path:
        """读取算例, 路径为空时生成对应模板并返回模板路径"""
        case_config = CaseConfigLoader.load(path)
        if isinstance(case_config, Path):
            return case_config
        case_path = Path(path).expanduser().resolve()
        for name in ("thermal", "diffusion", "mechanics", "materials", "coupling", "numerics"):
            if getattr(case_config.input_files, name) == "":
                raise NotImplementedError(f"{name} 输入卡模板尚未实现, 暂时请省略该节点")
        geometry_mesh = cls._load_geometry_mesh(case_config.input_files.geometry_mesh, case_path=case_path)
        if isinstance(geometry_mesh, Path):
            return geometry_mesh
        geometry_mesh_path, geometry_mesh_config = geometry_mesh
        return LoadedCase(
            case_path=case_path,
            case_config=case_config,
            geometry_mesh_path=geometry_mesh_path,
            geometry_mesh_config=geometry_mesh_config,
        )

    @classmethod
    def _load_geometry_mesh(
        cls,
        file: str | None,
        *,
        case_path: Path,
    ) -> tuple[Path, GeometryMeshCard] | Path:
        """空路径生成模板, 否则解析并检查几何与网格输入卡"""
        if file is None or not file.strip():
            return GeometryMeshCardTemplate.write(case_path.parent / "geometry_mesh_empty.xml")
        geometry_mesh_path = cls.resolve_required_input_file(file,input_name="geometry_mesh",case_path=case_path)
        geometry_mesh_config = GeometryMeshCardLoader.load(geometry_mesh_path)
        GeometryMeshExamine.examine(geometry_mesh_config)
        return geometry_mesh_path, geometry_mesh_config

    @staticmethod
    def resolve_required_input_file(
        file: str | None,
        *,
        input_name: str,
        case_path: Path,
    ) -> Path:
        """以 case XML 所在目录为基准解析必填输入卡路径"""
        if file is None:
            raise ValueError(f"{case_path}: input_files 中缺少必填输入卡 {input_name}")
        return XmlUtils.resolve_path(file, case_path.parent)

def load_case(path: str | Path | None = None) -> LoadedCase | Path:
    """读取完整算例或生成空路径对应的输入模板"""
    return CaseLoader.load(path)
