from pathlib import Path
import gmsh
import json
import numpy as np
from trimuse.input.geometry_mesh.config import GeometryMeshCard
from trimuse.geometry.gmsh_mesh import GmshMesh
from trimuse.geometry.ideal_multilayer_sphere import IdealMultilayerSphereBuilder
from trimuse.geometry.flat_multilayer_particle import FlatMultilayerParticleBuilder
from trimuse.geometry.stochastic_multilayer_particle import StochasticMultilayerParticleBuilder
from trimuse.geometry.topology import GeometryTopology, MeshInfo

class GeometryMeshBuilder:
    """统一管理 Gmsh 生命周期和几何网格构建流程"""

    @classmethod
    def build(cls, config: GeometryMeshCard, output_directory: str | Path, rng: np.random.Generator) -> MeshInfo:
        """目标 MSH 已存在则读取复用, 否则构建并写出网格"""
        mesh_path = Path(output_directory).expanduser().resolve() / "mesh" / f"{config.geometry.source}.msh"
        gmsh.initialize()
        try:
            if mesh_path.is_file():
                geometry_path = mesh_path.with_suffix(".geometry.json")
                if not geometry_path.is_file():
                    raise FileNotFoundError(f"已有网格缺少实际几何参数，请重新生成网格: {geometry_path}")
                geometry_data = json.loads(geometry_path.read_text(encoding="utf-8"))
                mesh_info = GmshMesh.read(mesh_path, config.mesh.output.formats)
                mesh_info.geometry_method = geometry_data["geometry_method"]
                mesh_info.center = tuple(geometry_data["center"])
                mesh_info.region_layers = {int(attribute): layers for attribute, layers in geometry_data["region_layers"].items()}
                print(f"复用已有网格: {mesh_path}", flush=True)
                return mesh_info
            gmsh.model.add("trimuse_geometry")
            topology = cls._build_geometry(config, rng)
            mesh_info = GmshMesh.generate(topology, config.mesh, mesh_path)
            geometry = getattr(config.geometry, config.geometry.source)
            mesh_info.geometry_method = config.geometry.source
            mesh_info.center = (geometry.center.x, geometry.center.y, geometry.center.z)
            mesh_info.region_layers = {mesh_info.region_attributes[name]: layers for name, layers in topology.region_layers.items()} # 把物理组名换成区域编号
            # 与网格一起保存实际轮廓，复用网格时不重新抽样随机几何。
            geometry_data = {"geometry_method": mesh_info.geometry_method, "center": mesh_info.center, "region_layers": mesh_info.region_layers}
            mesh_path.with_suffix(".geometry.json").write_text(json.dumps(geometry_data, ensure_ascii=False, indent=2), encoding="utf-8")
            return mesh_info
        finally:
            gmsh.finalize()

    @staticmethod
    def _build_geometry(config: GeometryMeshCard, rng: np.random.Generator) -> GeometryTopology:
        """按几何来源调用对应构造器"""
        zones = config.geometry.zones
        if config.geometry.source == "ideal_multilayer_sphere":
            sphere = config.geometry.ideal_multilayer_sphere
            return IdealMultilayerSphereBuilder.build(sphere, config.model.dimension, zones)
        elif config.geometry.source == "stochastic_multilayer_particle":
            particle = config.geometry.stochastic_multilayer_particle
            return StochasticMultilayerParticleBuilder.build(particle, config.model.dimension, rng, zones, config.mesh.space_sampler)
        elif config.geometry.source == "flat_multilayer_particle":
            return FlatMultilayerParticleBuilder.build(config.geometry.flat_multilayer_particle, config.model.dimension, zones)
