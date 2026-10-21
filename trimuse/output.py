from __future__ import annotations
from collections import Counter
from dataclasses import asdict, fields
from datetime import datetime
from pathlib import Path
from time import perf_counter
from typing import TYPE_CHECKING
from xml.etree import ElementTree as ET
from zoneinfo import ZoneInfo
import json

if TYPE_CHECKING:
    from _core import FemSystem
    from .input.xml_loader import InputCards

class OutputManager:
    """统一管理输入快照、C++ 场输出和模拟完成信息"""

    def __init__(self, input_cards: InputCards) -> None:
        self.input_cards = input_cards
        self.directory = Path(input_cards.chief.output.directory).expanduser().resolve()
        self.directory.mkdir(parents=True, exist_ok=True)
        self.completed_file = self.directory / "simulation_complete.flag"
        self.started_at = self._now()
        self.start_clock = perf_counter()
        self.mesh_statistics = None
        self.frame_count = 0
        self.field_output = None

    @staticmethod
    def _now() -> str:
        return datetime.now(ZoneInfo("Asia/Shanghai")).isoformat(timespec="seconds")

    def is_complete(self) -> bool:
        return self.completed_file.is_file()

    def record_mesh(self, mesh_path: str | Path) -> None:
        """统计实际导出的原始网格, 不统计后处理细分单元; 总数包含各维数单元"""
        from vtkmodules.vtkCommonDataModel import vtkCellTypes
        from vtkmodules.vtkIOLegacy import vtkUnstructuredGridReader

        reader = vtkUnstructuredGridReader()
        reader.SetFileName(str(Path(mesh_path).with_suffix(".vtk")))
        reader.Update()
        mesh = reader.GetOutput()
        counts = Counter(mesh.GetCellType(index) for index in range(mesh.GetNumberOfCells()))
        dimension = self.input_cards.geometry_mesh.model.dimension
        self.mesh_statistics = {
            "total_element_count": sum(counts.values()),
            "domain_element_count": sum(count for cell_type, count in counts.items() if vtkCellTypes.GetDimension(cell_type) == dimension),
            "boundary_element_count": sum(count for cell_type, count in counts.items() if vtkCellTypes.GetDimension(cell_type) == dimension - 1),
            "element_counts_by_type": {
                vtkCellTypes.GetClassNameFromTypeId(cell_type).removeprefix("vtk"): count
                for cell_type, count in sorted(counts.items())
            }
        }

    def initialize_field_output(self, fem_system: FemSystem) -> None:
        """只在求解分支创建 C++ 输出对象, 后处理分支不加载内核"""
        from _core import OutputManager as CoreOutputManager
        self.field_output = CoreOutputManager(fem_system, str(self.directory))

    def save(self, cycle: int, time: float, time_step: float) -> None:
        self.field_output.save(cycle, time, time_step)
        self.frame_count += 1

    def finish_simulation(self, simulation_time: float, step_count: int) -> None:
        """全部计算和场输出成功后写入 JSON 完成标记"""
        cards = self.input_cards
        info = {
            "status": "completed",
            "name": cards.chief.name,
            "random_seed": cards.chief.random_seed,
            "dimension": cards.geometry_mesh.model.dimension,
            "mesh": self.mesh_statistics,
            "fields": [item.name for item in cards.simulation.problem.field],
            "simulation_time_s": simulation_time,
            "configured_duration_s": sum(cards.simulation.problem.time_span),
            "time_steps": step_count,
            "output_frames": self.frame_count,
            "started_at": self.started_at,
            "finished_at": self._now(),
            "elapsed_seconds": perf_counter() - self.start_clock
        }
        temporary = self.completed_file.with_suffix(".tmp")
        temporary.write_text(json.dumps(info, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        temporary.replace(self.completed_file)

    @staticmethod
    def _text(value) -> str:
        if isinstance(value, list):
            return " ".join(OutputManager._text(item) for item in value)
        return str(value).lower() if isinstance(value, bool) else str(value)

    @classmethod
    def _node(cls, parent: ET.Element, tag: str, **attributes) -> ET.Element:
        """按名称或区域查找原节点, 保留模型参数的单位信息"""
        identity = {key: cls._text(value) for key, value in attributes.items() if key in ("name", "region")}
        element = next((child for child in parent.findall(tag) if all(child.get(key) == value for key, value in identity.items())), None)
        if element is None:
            element = ET.SubElement(parent, tag)
        element.attrib.update({key: cls._text(value) for key, value in attributes.items() if value is not None})
        return element

    @classmethod
    def _quantity(cls, parent: ET.Element, tag: str, value, unit: str) -> None:
        if value is not None:
            cls._node(parent, tag, value=value, unit=unit)

    @classmethod
    def _center(cls, parent: ET.Element, center) -> None:
        if center is not None:
            node = cls._node(parent, "center")
            for name in ("x", "y", "z"):
                cls._quantity(node, name, getattr(center, name), "[m]")

    @classmethod
    def _direction(cls, parent: ET.Element, tag: str, direction) -> None:
        if direction is not None:
            node = cls._node(parent, tag)
            cls._quantity(node, "theta", direction.theta, "[rad]")
            cls._quantity(node, "phi", direction.phi, "[rad]")

    @classmethod
    def _model(cls, element: ET.Element, model: str, parameters: dict) -> None:
        """配置中的值已换算到 si_unit, 保存时不再次转换"""
        element.set("model", model)
        units = {item.get("name"): item.get("si_unit") for item in element.findall("parameter")}
        for item in element.findall("parameter"):
            element.remove(item)
        for name, value in parameters.items():
            ET.SubElement(element, "parameter", name=name, value=cls._text(value), unit=units[name], si_unit=units[name])

    def write_input_cards(self) -> None:
        """保存可重新读取的 XML: 补全已知默认值, 不虚构未指定的物理参数"""
        cards = self.input_cards
        directory = self.directory / "input"
        directory.mkdir(parents=True, exist_ok=True)
        chief = ET.Element("trimuse_chief", name=cards.chief.name)
        self._node(chief, "random", seed=cards.chief.random_seed)
        # Loader 会追加 seed 子目录, 此处写回用户目录, 避免重读后重复追加。
        self._node(chief, "output", directory=self.directory.parent)
        self._node(chief, "workflow", mode=cards.chief.workflow.mode)
        input_files = self._node(chief, "input_files")
        writers = {
            "geometry_mesh": self._geometry_card,
            "material": self._material_card,
            "properties": self._properties_card,
            "thermal": self._thermal_card,
            "simulation": self._simulation_card,
            "postprocess": self._postprocess_card
        }
        for name, writer in writers.items():
            root = ET.parse(getattr(cards.chief.input_files, name)).getroot()
            writer(root, getattr(cards, name))
            ET.indent(root, space="    ")
            ET.ElementTree(root).write(directory / f"{name}.xml", encoding="utf-8", xml_declaration=True)
            self._node(input_files, name, file=f"./{name}.xml")
        ET.indent(chief, space="    ")
        ET.ElementTree(chief).write(directory / "chief.xml", encoding="utf-8", xml_declaration=True)

    @classmethod
    def _geometry_card(cls, root: ET.Element, card) -> None:
        dimension = card.model.dimension
        cls._node(root, "model", dimension=dimension)
        geometry = cls._node(root, "geometry", source=card.geometry.source)
        model = getattr(card.geometry, card.geometry.source)
        node = cls._node(geometry, card.geometry.source)
        cls._center(node, model.center)
        if card.geometry.source == "stochastic_multilayer_particle":
            surface = cls._node(node, "outer_surface")
            cls._quantity(surface, "batch_mean_radius", model.outer_surface.batch_mean_radius, "[m]")
            cls._quantity(surface, "target_sphericity", model.outer_surface.target_sphericity, "[1]")
            expansion = model.outer_surface.shape_expansion
            if expansion is not None:
                cls._node(surface, "shape_expansion", **asdict(expansion))
        region_nodes = cls._node(node, "regions")
        for region in model.regions.region:
            item = cls._node(region_nodes, "region", name=region.name)
            cls._node(item, "outer_boundary", name=region.outer_boundary.name)
            for field in fields(region):
                if field.name not in ("name", "outer_boundary"):
                    unit = "[1]" if field.name == "unwrapping_coefficient" else "[m]"
                    cls._quantity(item, field.name, getattr(region, field.name), unit)
        zones = card.geometry.zones
        zone_root = cls._node(geometry, "zones", outer_boundary=zones.outer_boundary)
        for zone in zones.zone:
            item = cls._node(zone_root, "zone", name=zone.name)
            for field in fields(zone):
                if field.name != "name":
                    cls._quantity(item, field.name, getattr(zone, field.name), "[rad]")
        cls._node(zone_root, "residual_zone", name=zones.residual_zone)
        mesh = cls._node(root, "mesh")
        for item in mesh.findall("output"):
            mesh.remove(item)
        sampler = asdict(card.mesh.space_sampler)
        if card.geometry.source == "stochastic_multilayer_particle" and dimension > 1:
            degree = model.outer_surface.shape_expansion.max_degree
            if sampler["theta_interval_count"] is None:
                sampler["theta_interval_count"] = max(128, 8 * degree) if dimension == 2 else max(16, degree + 1)
            if dimension == 3 and sampler["phi_count"] is None:
                sampler["phi_count"] = max(32, 2 * degree + 1)
        cls._node(mesh, "space_sampler", **sampler)
        cls._node(mesh, "element", **asdict(card.mesh.element))
        cls._node(mesh, "gmsh", **asdict(card.mesh.gmsh))
        cls._node(mesh, "output", formats=card.mesh.output.formats)
        sizing = cls._node(mesh, "sizing")
        global_node = cls._node(sizing, "global")
        for name, value in asdict(card.mesh.sizing.global_).items():
            cls._quantity(global_node, name, value, "[m]")
        for region in card.mesh.sizing.region:
            cls._quantity(cls._node(sizing, "region", name=region.name), "target_edge_length", region.target_edge_length, "[m]")

    @classmethod
    def _material_card(cls, root: ET.Element, card) -> None:
        for material in card.mat:
            node = cls._node(root, "mat", name=material.name)
            for property in material.properties:
                category, name = property.property_name.split(".", 1)
                cls._model(cls._node(cls._node(node, category), name), property.model, property.parameters)
        assignments = cls._node(root, "assign")
        for region in card.assign.region:
            cls._node(assignments, "region", **asdict(region))

    @classmethod
    def _properties_card(cls, root: ET.Element, card) -> None:
        for category in ("external_property", "derived_property"):
            for property in getattr(card, category):
                node = cls._node(root, category, name=property.name)
                for scope in ("region", "zone"):
                    for item in getattr(property, scope):
                        cls._model(cls._node(node, scope, name=item.name), item.model, item.parameters)
                if property.global_ is not None:
                    cls._model(cls._node(node, "global"), property.global_.model, property.global_.parameters)

    @classmethod
    def _thermal_card(cls, root: ET.Element, card) -> None:
        initial = cls._node(root, "initial")
        for item in card.initial.temperature:
            cls._model(cls._node(initial, "temperature", region=item.region), item.model, item.parameters)
        groups = (("boundaries", "boundary", card.boundary),
                  ("interfaces", "interface", card.interfaces.interface),
                  ("source", "region", card.source.region))
        for group, tag, configs in groups:
            node = cls._node(root, group)
            for item in configs:
                cls._model(cls._node(node, tag, name=item.name), item.model, item.parameters)

    @classmethod
    def _simulation_card(cls, root: ET.Element, card) -> None:
        problem = cls._node(root, "problem")
        cls._quantity(problem, "time_span", card.problem.time_span, "[s]")
        cls._quantity(problem, "min_time_step", card.problem.min_time_step, "[s]")
        for field in card.problem.field:
            theta = field.theta if field.theta is not None else 1.0 if field.model == "transient" else None
            cls._node(problem, "field", name=field.name, model=field.model, theta=theta)
        solver = cls._node(root, "solver")
        cls._node(solver, "quadrature", **asdict(card.solver.quadrature))
        for field in card.solver.field:
            node = cls._node(solver, "field", name=field.name)
            cls._node(node, "fem_space", **asdict(field.fem_space))
            cls._node(node, "linear_solver", **asdict(field.linear_solver))
            nonlinear = asdict(field.nonlinear_solver)
            tolerance = nonlinear.pop("absolute_tolerance")
            cls._quantity(cls._node(node, "nonlinear_solver", **nonlinear), "absolute_tolerance", tolerance, "[K]")
        coupling = cls._node(root, "coupling", exchange_interval=card.coupling.exchange_interval)
        for node in coupling.findall("step"):
            coupling.remove(node)
        for step in card.coupling.step:
            ET.SubElement(coupling, "step", {key: cls._text(value) for key, value in asdict(step).items()})
        cls._quantity(cls._node(root, "output"), "write_interval", card.output.write_interval, "[s]")

    @classmethod
    def _postprocess_card(cls, root: ET.Element, card) -> None:
        for group, tag in (("points", "point"), ("lines", "line"), ("sections", "section")):
            node = cls._node(root, group)
            for item in getattr(card, group):
                element = cls._node(node, tag, name=item.name)
                if group != "lines":
                    cls._center(element, item.center)
                else:
                    for center in element.findall("center"):
                        element.remove(center)
                if group == "lines":
                    element.set("sample_count", str(item.sample_count))
                    cls._direction(element, "direction", item.direction)
                elif group == "sections":
                    cls._direction(element, "normal", item.normal)
        cls._node(root, "properties", names=card.properties)
