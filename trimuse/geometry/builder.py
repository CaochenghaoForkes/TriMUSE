"""几何与网格构建总调度器"""

from pathlib import Path
import gmsh
import numpy as np
from trimuse.input.geometry_mesh_config import GeometryMeshCard
from trimuse.geometry.gmsh_mesh import GmshMesh
from trimuse.geometry.ideal_multilayer_sphere import IdealMultilayerSphereBuilder
from trimuse.geometry.stochastic_multilayer_particle import StochasticMultilayerParticleBuilder
from trimuse.geometry.topology import GeometryTopology

class GeometryMeshBuilder:
    """统一管理Gmsh生命周期和几何网格构建流程"""

    @classmethod
    def build(cls, config: GeometryMeshCard, output_directory: str | Path, rng: np.random.Generator) -> Path:
        """根据已检查的配置构建并写出网格"""
        gmsh.initialize()
        try:
            gmsh.model.add("trimuse_geometry")
            topology = cls._build_geometry(config, rng)
            output_path = cls._resolve_output_path(config.mesh.output.file, output_directory)
            return GmshMesh.generate(topology, config.mesh, output_path)
        finally:
            gmsh.finalize()

    @staticmethod
    def _build_geometry(config: GeometryMeshCard, rng: np.random.Generator) -> GeometryTopology:
        """按几何来源调用对应构造器"""
        if config.geometry.source == "ideal_multilayer_sphere":
            sphere = config.geometry.ideal_multilayer_sphere
            assert sphere is not None
            return IdealMultilayerSphereBuilder.build(sphere, config.model.dimension)
        if config.geometry.source == "stochastic_multilayer_particle":
            particle = config.geometry.stochastic_multilayer_particle
            assert particle is not None
            return StochasticMultilayerParticleBuilder.build(particle, config.model.dimension, rng)
        raise ValueError(f"暂不支持 geometry source {config.geometry.source!r}")

    @staticmethod
    def _resolve_output_path(file: str, output_directory: str | Path) -> Path:
        """在外部指定的网格输出目录下解析文件名"""
        path = Path(file).expanduser()
        if path.is_absolute():
            raise ValueError("几何网格输出文件必须使用相对路径")
        return (Path(output_directory).expanduser().resolve() / path).resolve()
