from __future__ import annotations
from math import cos, sin
from pathlib import Path

from vtkmodules.vtkCommonDataModel import vtkStaticCellLocator

from trimuse.input.xml_loader import InputCards
from trimuse.input.postprocess.config import PostprocessCard
from .output_reader import OutputReader
from .point_plotter import PointPlotter
from .line_plotter import LinePlotter
from .section_plotter import SectionPlotter
from .material_regions import MaterialRegions


class PostprocessManager:
    """接收已读取的输入卡, 调度点、线和截面后处理, 不依赖有限元内核对象"""

    def __init__(self, input_cards: InputCards) -> None:
        self.config = input_cards.postprocess
        self.output_directory = Path(input_cards.chief.output.directory).expanduser().resolve()
        self.postprocess_directory = self.output_directory / "postprocess"
        self.dimension = input_cards.geometry_mesh.model.dimension
        geometry = input_cards.geometry_mesh.geometry
        center = getattr(geometry, geometry.source).center
        self.center = (center.x, center.y if self.dimension > 1 else 0.0, center.z if self.dimension > 2 else 0.0)

        self.reader = OutputReader(self.output_directory, self.config.properties)
        self.point_plotter = PointPlotter(self.output_directory, self.postprocess_directory, self.reader, self.center)
        self.line_plotter = LinePlotter(self.output_directory, self.postprocess_directory, self.reader, self.center)
        self.section_plotter = SectionPlotter(self.output_directory, self.postprocess_directory, self.reader, self.center)
        self.material_regions = None
        if self.dimension > 1:
            mesh_path = self.output_directory / "mesh" / f"{geometry.source}.vtk"
            materials = {region.name: region.mat for region in input_cards.material.assign.region}
            self.material_regions = MaterialRegions(mesh_path, self.dimension, materials)
        self.lines = list(self.config.lines)
        self.sections = list(self.config.sections)
        if self.dimension == 1 and not self.lines:
            self.lines.append(PostprocessCard.Line(name="radial", sample_count=100))
        if self.dimension == 2 and not self.sections:
            self.sections.append(PostprocessCard.Section(name="whole_domain"))
        # 每个属性使用自己的首帧输出网格, 同一属性的全部位置和时间帧共用定位器。
        self.locators: dict[str, vtkStaticCellLocator] = {}
        for name in self.config.properties:
            mesh = self.reader.read_frame(name, 0)
            locator = vtkStaticCellLocator()
            locator.SetDataSet(mesh)
            locator.BuildLocator()
            self.locators[name] = locator

    @staticmethod
    def coordinates(center: PostprocessCard.Center) -> tuple[float, float, float]:
        """将输入卡的缺省坐标分量补为零"""
        return (center.x, center.y if center.y is not None else 0.0, center.z if center.z is not None else 0.0)

    def direction(self, direction: PostprocessCard.Direction | None) -> tuple[float, float, float]:
        """输入角度已为 rad: 2D theta 从 +y 起算, 3D theta 从 +z 起算"""
        if self.dimension == 1:
            return (1.0, 0.0, 0.0)
        if self.dimension == 2:
            return (sin(direction.theta), cos(direction.theta), 0.0)
        return (sin(direction.theta) * cos(direction.phi), sin(direction.theta) * sin(direction.phi), cos(direction.theta))

    def process_points(self, property_name: str) -> None:
        """每个点保存一条时间序列及曲线"""
        locator = self.locators[property_name]
        for point in self.config.points:
            rows = self.point_plotter.extract(self.coordinates(point.center), property_name, locator)
            data = self.point_plotter.save(rows, property_name, point.name)
            self.point_plotter.plot(data, property_name, point.name)

    def process_lines(self, property_name: str) -> None:
        """每条线逐帧保存空间分布及带时间标注的曲线"""
        locator = self.locators[property_name]
        for line in self.lines:
            center = self.coordinates(line.center) if line.center is not None else self.center
            direction = self.direction(line.direction)
            for frame_directory, time, data in self.line_plotter.extract(center, direction, line.sample_count, property_name, locator):
                self.line_plotter.save(data, property_name, line.name, frame_directory)
                self.line_plotter.plot(data, property_name, line.name, frame_directory, time)

    def process_sections(self, property_name: str) -> None:
        """二维默认全截面, 三维使用指定中心和法线, 逐帧保存云图和数据"""
        for section in self.sections:
            center = self.coordinates(section.center) if section.center is not None else self.center
            normal = (0.0, 0.0, 1.0) if self.dimension == 2 else self.direction(section.normal)
            regions = self.material_regions.section(center, normal)
            for frame_directory, time, data in self.section_plotter.extract(center, normal, property_name):
                self.section_plotter.save(data, property_name, section.name, frame_directory)
                self.section_plotter.plot(data, property_name, section.name, frame_directory, time, normal, regions)

    def run(self) -> None:
        """按属性依次完成点、线和截面后处理"""
        for name in self.config.properties:
            self.process_points(name)
            self.process_lines(name)
            self.process_sections(name)
