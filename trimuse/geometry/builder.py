from pathlib import Path
import gmsh
import numpy as np
from trimuse.input.geometry_mesh.config import GeometryMeshCard
from trimuse.geometry.gmsh_mesh import GmshMesh
from trimuse.geometry.ideal_multilayer_sphere import IdealMultilayerSphereBuilder
from trimuse.geometry.stochastic_multilayer_particle import StochasticMultilayerParticleBuilder
from trimuse.geometry.topology import GeometryTopology, MeshInfo

class GeometryMeshBuilder:
    """统一管理 Gmsh 生命周期和几何网格构建流程"""

    @classmethod
    def build(cls, config: GeometryMeshCard, output_directory: str | Path, rng: np.random.Generator) -> MeshInfo:
        """目标 MSH 已存在则读取复用, 否则构建并写出网格"""
        mesh_path = Path(output_directory).expanduser().resolve() / "mesh" / config.mesh.output.file
        gmsh.initialize()
        try:
            if mesh_path.is_file():
                mesh_info = GmshMesh.read(mesh_path)
                print(f"复用已有网格: {mesh_path}", flush=True)
                return mesh_info
            gmsh.model.add("trimuse_geometry")
            topology = cls._build_geometry(config, rng)
            mesh_info = GmshMesh.generate(topology, config.mesh, output_directory)
            return mesh_info
        finally:
            gmsh.finalize()

    @staticmethod
    def _build_geometry(config: GeometryMeshCard, rng: np.random.Generator) -> GeometryTopology:
        """按几何来源调用对应构造器"""
        surface_zones = config.geometry.surface_zones
        if config.geometry.source == "ideal_multilayer_sphere":
            sphere = config.geometry.ideal_multilayer_sphere
            return IdealMultilayerSphereBuilder.build(sphere, config.model.dimension, surface_zones)
        elif config.geometry.source == "stochastic_multilayer_particle":
            particle = config.geometry.stochastic_multilayer_particle
            return StochasticMultilayerParticleBuilder.build(particle, config.model.dimension, rng, surface_zones, config.mesh.space_sampler)
