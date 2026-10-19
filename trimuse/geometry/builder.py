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
        """根据已检查的配置构建并写出网格"""
        gmsh.initialize()
        try:
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
