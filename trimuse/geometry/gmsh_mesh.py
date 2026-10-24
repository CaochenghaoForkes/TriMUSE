from pathlib import Path
import gmsh
from trimuse.input.geometry_mesh.config import GeometryMeshCard
from trimuse.geometry.topology import GeometryTopology, MeshInfo
MeshConfig = GeometryMeshCard.Mesh

class GmshMesh:
    """根据几何拓扑和网格配置生成 Gmsh 网格"""

    _ENTITY_LISTS = {1: "CurvesList", 2: "SurfacesList", 3: "VolumesList"}
    _ALGORITHMS_3D = {"del3d": 1, "front3d": 4, "mmg3d": 7, "hxt": 10}
    _OPTIMIZERS = {"gmsh": "", "netgen": "Netgen"}

    @classmethod
    def read(cls, path: str | Path, formats: list[str] | tuple[str, ...] = ("vtk",)) -> MeshInfo:
        """读取已有网格及其物理组名称编号, 补导出缺失格式, 不重新划分网格"""
        path = Path(path).expanduser().resolve()
        gmsh.open(str(path))
        cls._write_formats(path, formats, missing_only=True)
        dimension = gmsh.model.getDimension()
        regions = {gmsh.model.getPhysicalName(dim, tag): tag for dim, tag in gmsh.model.getPhysicalGroups(dimension)}
        boundaries = {gmsh.model.getPhysicalName(dim, tag): tag for dim, tag in gmsh.model.getPhysicalGroups(dimension - 1)}
        return MeshInfo(path=path, dimension=dimension, region_attributes=regions, boundary_attributes=boundaries)

    @classmethod
    def generate(cls, topology: GeometryTopology, config: MeshConfig, mesh_path: str | Path) -> MeshInfo:
        """使用已检查的配置生成网格, 写入输出根目录下的 mesh 子目录"""
        region_attributes, boundary_attributes = cls._add_physical_groups(topology)
        cls._set_mesh_sizes(topology, config.sizing)
        cls._set_algorithm(topology.dimension, config.gmsh.algorithm_3d)
        gmsh.model.mesh.generate(topology.dimension)
        cls._optimize(topology.dimension, config.gmsh.optimizer)
        cls._set_order(config.element.geometry_order, config.gmsh.optimizer)
        cls._check_mesh(topology)
        path = cls._write(mesh_path, config.output.formats)
        mesh_info = MeshInfo(
            path=path,
            dimension=topology.dimension,
            region_attributes=region_attributes,
            boundary_attributes=boundary_attributes,
        )
        return mesh_info

    @staticmethod
    def _add_physical_groups(topology: GeometryTopology) -> tuple[dict[str, int], dict[str, int]]:
        """注册 Gmsh 物理组并返回名称与属性编号的映射"""
        region_attributes = {}
        boundary_attributes = {}
        groups = ((topology.dimension, topology.regions, region_attributes), (topology.dimension - 1, topology.boundaries, boundary_attributes))
        for dimension, named_entities, attributes in groups:
            for name, entity_tags in named_entities.items():
                if not entity_tags:
                    raise RuntimeError(f"物理组 {name!r} 不包含任何几何实体")
                group_tag = gmsh.model.addPhysicalGroup(dimension, entity_tags)
                gmsh.model.setPhysicalName(dimension, group_tag, name)
                attributes[name] = group_tag
        return region_attributes, boundary_attributes

    @classmethod
    def _set_mesh_sizes(cls, topology: GeometryTopology, sizing: MeshConfig.Sizing) -> None:
        """设置全局网格尺寸范围和材料区域目标尺寸"""
        global_sizing = sizing.global_
        if global_sizing.min_edge_length is not None:
            gmsh.option.setNumber("Mesh.MeshSizeMin", global_sizing.min_edge_length)
        if global_sizing.max_edge_length is not None:
            gmsh.option.setNumber("Mesh.MeshSizeMax", global_sizing.max_edge_length)

        region_fields = [] # 保存各区域尺寸场的编号
        entity_list = cls._ENTITY_LISTS[topology.dimension]
        for region in sizing.region:
            value_field = gmsh.model.mesh.field.add("MathEval") # 创建数学表达式场，返回其编号
            gmsh.model.mesh.field.setString(value_field, "F", repr(region.target_edge_length)) # 设置其表达式 F, F 为常数(float->string)
            region_field = gmsh.model.mesh.field.add("Restrict") # 用另一个场限制其作用域
            gmsh.model.mesh.field.setNumber(region_field, "InField", value_field) # 告诉限制场要限制的是刚才那个 MathEval 场
            gmsh.model.mesh.field.setNumbers(region_field, entity_list, topology.regions[region.name]) # 指定生效的几何实体
            gmsh.model.mesh.field.setNumber(region_field, "IncludeBoundary", 1) # 1 表示不仅区域内部该区域的边界也使用这个尺寸场
            region_fields.append(region_field) # 将这个限制后的场编号加入列表，供后面统一组合使用

        if not region_fields:
            return
        background_field = region_fields[0] # 如果只有一个区域尺寸场 则可以直接使用它
        if len(region_fields) > 1: # 多个场时 创建最小值的组合场
            background_field = gmsh.model.mesh.field.add("Min") 
            gmsh.model.mesh.field.setNumbers(background_field, "FieldsList", region_fields) # 告诉他需要组合哪些场 (前面的 Restrict 已经限制了各场的作用范围，所以不会简单地让整个颗粒都使用所有区域中最小的尺寸, 但共同边界上会使用最小的那个)
        gmsh.model.mesh.field.setAsBackgroundMesh(background_field) # 正式启用该背景场

    @classmethod
    def _set_algorithm(cls, dimension: int, algorithm_3d: str) -> None:
        """设置三维非结构网格算法"""
        if dimension == 3:
            gmsh.option.setNumber("Mesh.Algorithm3D", cls._ALGORITHMS_3D[algorithm_3d])

    @classmethod
    def _optimize(cls, dimension: int, optimizer: str) -> None:
        """优化三维一次网格"""
        if dimension == 3 and optimizer != "none":
            gmsh.model.mesh.optimize(cls._OPTIMIZERS[optimizer])

    @staticmethod
    def _set_order(order: int, optimizer: str) -> None:
        """设置几何阶次并按需优化高阶网格"""
        if order == 1:
            return # 默认生成的网格就是 1 阶的
        gmsh.model.mesh.setOrder(order) # 提升网格阶数 使球面边界不再只是直边、平面片的近似
        if optimizer != "none":
            gmsh.model.mesh.optimize("HighOrder") # 只要没有关闭优化，就执行 Gmsh 的 HighOrder 高阶网格优化，调整高阶节点位置，改善弯曲单元质量

    @staticmethod
    def _check_mesh(topology: GeometryTopology) -> None:
        """确认每个材料区域均已生成最高维单元"""
        for name, entity_tags in topology.regions.items():
            element_count = 0
            for entity_tag in entity_tags:
                _, element_tags, _ = gmsh.model.mesh.getElements(topology.dimension, entity_tag)
                element_count += sum(len(tags) for tags in element_tags)
            if element_count == 0:
                raise RuntimeError(f"材料区域 {name!r} 未生成 {topology.dimension}D 单元")

    @classmethod
    def _write(cls, mesh_path: str | Path, formats: list[str] | tuple[str, ...] = ("vtk",)) -> Path:
        """输出 MSH 2.2 ASCII、VTK 及用户要求的附加格式"""
        path = Path(mesh_path).expanduser().resolve()
        path.parent.mkdir(parents=True, exist_ok=True)
        gmsh.option.setNumber("Mesh.MshFileVersion", 2.2) # 指定输出的 .msh 使用 MSH 2.2 格式
        gmsh.option.setNumber("Mesh.Binary", 0) # 使用 ASCII 文本输出
        cls._write_formats(path, formats)
        return path

    @staticmethod
    def _write_formats(path: Path, formats: list[str] | tuple[str, ...], missing_only: bool = False) -> None:
        """同名输出各格式, 重复格式只写一次; BDF 使用物理组编号作为标识"""
        for format_name in dict.fromkeys(("msh", "vtk", *formats)): # 遍历 msh vtk 以及 formats 中包含的其他格式
            target = path.with_suffix(f".{format_name}") # 替换扩展名，但不修改 path 本身 如 mesh/particle.msh ...
            if missing_only and target.is_file(): # 按需跳过已有文件
                continue
            if format_name == "bdf": # bdf 的 Nastran 网格格式的专门设置
                previous_tag_type = gmsh.option.getNumber("Mesh.SaveElementTagType") # 先保存 Gmsh 原本的设置
                previous_field_format = gmsh.option.getNumber("Mesh.BdfFieldFormat") # 先保存 Gmsh 原本的设置
                gmsh.option.setNumber("Mesh.SaveElementTagType", 2) # 选择使用物理组编号作为导出标识，而不是几何实体编号，方便外部软件按材料区域、边界分组识别
                gmsh.option.setNumber("Mesh.BdfFieldFormat", 2) # 选择 BDF 的 large 大字段格式，给坐标等数值提供更宽的字段，减少输出精度受字段宽度限制的影响
                gmsh.write(str(target)) # Gmsh 根据文件扩展名选择对应的写出格式
                gmsh.option.setNumber("Mesh.SaveElementTagType", previous_tag_type) # 恢复原来的全局设置
                gmsh.option.setNumber("Mesh.BdfFieldFormat", previous_field_format) # 恢复原来的全局设置
            else:
                gmsh.write(str(target))
