from __future__ import annotations
from dataclasses import dataclass
from pathlib import Path

from .chief.config import ChiefCard, ChiefCardLoader
from .chief.examine import ChiefCardExamine
from .geometry_mesh.config import GeometryMeshCard, GeometryMeshCardLoader
from .geometry_mesh.examine import GeometryMeshCardExamine
from .material.config import MaterialCard, MaterialCardLoader
from .material.examine import MaterialCardExamine
from .properties.config import PropertiesCard, PropertiesCardLoader
from .properties.examine import PropertiesCardExamine
from .thermal.config import ThermalCard, ThermalCardLoader
from .thermal.examine import ThermalCardExamine
from .simulation.config import SimulationCard, SimulationCardLoader
from .simulation.examine import SimulationCardExamine
from .postprocess.config import PostprocessCard, PostprocessCardLoader
from .postprocess.examine import PostprocessCardExamine
from .examine import InputCardsExamine

@dataclass
class InputCards:
    """通过检查的总调度与各模块输入配置"""

    chief: ChiefCard
    geometry_mesh: GeometryMeshCard
    material: MaterialCard
    properties: PropertiesCard
    thermal: ThermalCard
    simulation: SimulationCard
    postprocess: PostprocessCard

class XmlLoader:
    """统一加载输入卡, 空路径由对应读取器生成模板并退出程序"""

    @classmethod
    def load(cls, chief_path: str | Path | None = None) -> InputCards:
        """读取总调度与子卡, 空路径生成对应模板"""
        chief = ChiefCardLoader.load(chief_path)
        ChiefCardExamine.examine(chief)
        geometry_mesh = cls._load_geometry_mesh(chief.input_files.geometry_mesh)
        material = cls._load_material(chief.input_files.material)
        properties = cls._load_properties(chief.input_files.properties)
        thermal = cls._load_thermal(chief.input_files.thermal)
        simulation = cls._load_simulation(chief.input_files.simulation)
        postprocess = cls._load_postprocess(chief.input_files.postprocess)
        InputCardsExamine.examine(geometry_mesh, material, properties, thermal, postprocess)
        input_cards = InputCards(chief=chief, geometry_mesh=geometry_mesh, material=material, properties=properties, thermal=thermal, simulation=simulation, postprocess=postprocess)
        return input_cards

    @staticmethod
    def _load_geometry_mesh(path: str | None) -> GeometryMeshCard:
        """读取并检查配置, 空路径在当前工作目录生成模板并退出"""
        geometry_mesh = GeometryMeshCardLoader.load(path)
        GeometryMeshCardExamine.examine(geometry_mesh)
        return geometry_mesh

    @staticmethod
    def _load_material(path: str | None) -> MaterialCard:
        """读取并检查材料卡, 空路径生成模板并退出"""
        material = MaterialCardLoader.load(path)
        MaterialCardExamine.examine(material)
        return material

    @staticmethod
    def _load_properties(path: str | None) -> PropertiesCard:
        """读取并检查外部量和派生量输入卡"""
        properties = PropertiesCardLoader.load(path)
        PropertiesCardExamine.examine(properties)
        return properties

    @staticmethod
    def _load_thermal(path: str | None) -> ThermalCard:
        """读取并检查热卡, 空路径生成模板并退出"""
        thermal = ThermalCardLoader.load(path)
        ThermalCardExamine.examine(thermal)
        return thermal

    @staticmethod
    def _load_simulation(path: str | None) -> SimulationCard:
        """读取并检查仿真配置, 空路径生成模板并退出"""
        simulation = SimulationCardLoader.load(path)
        SimulationCardExamine.examine(simulation)
        return simulation

    @staticmethod
    def _load_postprocess(path: str | None) -> PostprocessCard:
        """读取并检查后处理卡, 空路径生成模板并退出"""
        postprocess = PostprocessCardLoader.load(path)
        PostprocessCardExamine.examine(postprocess)
        return postprocess

def load_input_cards(chief_path: str | Path | None = None) -> InputCards:
    """对外的输入加载入口"""
    return XmlLoader.load(chief_path)
