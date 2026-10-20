from __future__ import annotations
from math import cos, sin
from pathlib import Path
import numpy as np
from vtkmodules.vtkCommonDataModel import vtkStaticCellLocator

from trimuse.input.xml_loader import InputCards
from trimuse.input.postprocess.config import PostprocessCard
from .output_reader import OutputReader
from .point_plotter import PointPlotter
from .line_plotter import LinePlotter
from .section_plotter import SectionPlotter
from .boundary_geometry import BoundaryGeometry
from .animation_writer import save_gif
from .plot_style import BOUNDARY_COLORS, coordinate_labels, direction_labels


class PostprocessManager:
    """接收已读取的输入卡, 调度点、线和截面后处理, 不依赖有限元内核对象"""

    def __init__(self, input_cards: InputCards) -> None:
        # 基本配置输入
        self.config = input_cards.postprocess
        self.property_metadata = {"temperature": {"unit": "K", "color": "jet"}}
        self.output_directory = Path(input_cards.chief.output.directory).expanduser().resolve()
        self.postprocess_directory = self.output_directory / "postprocess"
        self.dimension = input_cards.geometry_mesh.model.dimension
        geometry = input_cards.geometry_mesh.geometry
        center = getattr(geometry, geometry.source).center
        self.center = (center.x, center.y if self.dimension > 1 else 0.0, center.z if self.dimension > 2 else 0.0)
        # 初始化类对象
        self.reader = OutputReader(self.output_directory, self.config.properties)
        self.point_plotter = PointPlotter(self.output_directory, self.postprocess_directory, self.reader, self.center, self.property_metadata)
        self.line_plotter = LinePlotter(self.output_directory, self.postprocess_directory, self.reader, self.center, self.property_metadata)
        self.section_plotter = SectionPlotter(self.output_directory, self.postprocess_directory, self.reader, self.center, self.property_metadata)
        self.section_plotter.annotation_fontsize = 10 if self.dimension == 3 else 12
        mesh_path = self.output_directory / "mesh" / f"{geometry.source}.vtk"
        self.boundary_geometry = BoundaryGeometry(mesh_path, self.dimension)
        colors = {name: BOUNDARY_COLORS[index % len(BOUNDARY_COLORS)] for index, (name, _) in enumerate(self.boundary_geometry.boundaries)}
        self.line_plotter.boundary_colors = colors
        self.section_plotter.boundary_colors = colors
        self.animation_jobs: list[tuple[list[Path], Path]] = []
        self.lines = list(self.config.lines)
        self.sections = list(self.config.sections)
        if self.dimension == 1 and not self.lines:
            self.lines.append(PostprocessCard.Line(name="radial", sample_count=100))
        if self.dimension == 2 and not self.sections:
            self.sections.append(PostprocessCard.Section(name="whole_domain"))
        # 每个属性使用自己的首帧输出网格, 同一属性的全部位置和时间帧共用定位器
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
            self.point_plotter.plot(data, property_name, point.name, self.coordinates(point.center))

    def process_lines(self, property_name: str) -> None:
        """每条线逐帧保存空间分布及带时间标注的曲线"""
        locator = self.locators[property_name]
        for line in self.lines:
            direction = self.direction(line.direction)
            annotation = direction_labels(line.direction.theta, line.direction.phi if self.dimension == 3 else None) if line.direction is not None else []
            boundaries = self.boundary_geometry.radial_intersections(self.center, direction)
            frames = []
            lower, upper = float("inf"), float("-inf")
            for frame_directory, time, data in self.line_plotter.extract(direction, line.sample_count, property_name, locator):
                path = self.line_plotter.save(data, property_name, line.name, frame_directory)
                frames.append((frame_directory, time, path))
                lower, upper = min(lower, data[:, 5].min()), max(upper, data[:, 5].max())
            images = []
            for frame_directory, time, path in frames:
                data = np.loadtxt(path, delimiter=",", skiprows=1, ndmin=2)
                images.append(self.line_plotter.plot(data, property_name, line.name, frame_directory, time, boundaries, (lower, upper), annotation))
            self.animation_jobs.append((images, images[0].parent / "animation.gif"))

    def process_sections(self, property_name: str) -> None:
        """二维默认全截面, 三维使用指定中心和法线, 逐帧保存云图和数据"""
        for section in self.sections:
            center = self.coordinates(section.center) if section.center is not None else self.center
            normal = (0.0, 0.0, 1.0) if self.dimension == 2 else self.direction(section.normal)
            angles = direction_labels(0.0, 0.0) if self.dimension == 2 else direction_labels(section.normal.theta, section.normal.phi)
            annotation = angles + coordinate_labels(center)
            boundaries = self.boundary_geometry.section(center, normal)
            # 第一遍保存数据并统计整个序列的范围; 第二遍绘图不保留所有帧的网格。
            lower, upper = float("inf"), float("-inf")
            for frame_directory, time, data in self.section_plotter.extract(center, normal, property_name):
                self.section_plotter.save(data, property_name, section.name, frame_directory)
                minimum, maximum = self.section_plotter.value_range(data, property_name)
                lower, upper = min(lower, minimum), max(upper, maximum)
            images = []
            for frame_directory, time, data in self.section_plotter.extract(center, normal, property_name):
                images.append(self.section_plotter.plot(data, property_name, section.name, frame_directory, time, normal, boundaries, (lower, upper), annotation))
            self.animation_jobs.append((images, images[0].parent / "animation.gif"))

    def run(self) -> None:
        """按属性依次完成点、线和截面后处理"""
        self.animation_jobs.clear()
        for name in self.config.properties:
            self.process_points(name)
            self.process_lines(name)
            self.process_sections(name)
        for images, path in self.animation_jobs:
            save_gif(images, path)
