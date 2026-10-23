"""XML 模板导出及内核模型清单覆盖检查。"""
from contextlib import redirect_stdout
import importlib
import io
import os
from pathlib import Path
import re
import tempfile
import unittest
from xml.etree import ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
INPUT = ROOT / "trimuse/input"
LOADERS = {
    "chief": "ChiefCardLoader", "geometry_mesh": "GeometryMeshCardLoader",
    "material": "MaterialCardLoader", "properties": "PropertiesCardLoader",
    "simulation": "SimulationCardLoader", "thermal": "ThermalCardLoader",
    "radioactivity": "RadioactivityCardLoader", "postprocess": "PostprocessCardLoader",
}


class XmlTemplateTests(unittest.TestCase):
    def test_empty_paths_copy_exact_bytes_and_refuse_overwrite(self):
        previous = Path.cwd()
        try:
            with tempfile.TemporaryDirectory() as directory, redirect_stdout(io.StringIO()):
                os.chdir(directory)
                for name, class_name in LOADERS.items():
                    loader = getattr(importlib.import_module(f"trimuse.input.{name}.config"), class_name)
                    source = INPUT / name / f"{name}.xml"
                    ET.parse(source)
                    for index, empty in enumerate((None, "", "  ")):
                        with self.subTest(name=name, empty=empty):
                            folder = Path(directory) / name / str(index)
                            folder.mkdir(parents=True)
                            os.chdir(folder)
                            with self.assertRaises(SystemExit):
                                loader.load(empty)
                            exported = folder / f"{name}_empty.xml"
                            self.assertEqual(exported.read_bytes(), source.read_bytes())
                            with self.assertRaises(FileExistsError):
                                loader.load(empty)
                            self.assertEqual(exported.read_bytes(), source.read_bytes())
        finally:
            os.chdir(previous)

    def test_no_python_string_templates_remain(self):
        self.assertEqual(list(INPUT.glob("*/template.py")), [])
        self.assertEqual(len(list(INPUT.glob("*/*.xml"))), len(LOADERS))

    def test_material_models_cover_cpp_registrations(self):
        text = (INPUT / "material/material.xml").read_text()
        for path in (ROOT / "cpp/properties/material").rglob("*_model_manager.cpp"):
            source = path.read_text()
            models = re.findall(r'registry_\.register_model<.*>\("([^"]+)"', source)
            if "register_common_models<" in source:
                models += ["common.constant", "common.time_series"]
            if not models:
                continue
            property_name = path.parent.name
            for model in models:
                with self.subTest(property=property_name, model=model):
                    self.assertIn(f'<{property_name} model="{model}"', text)

    def test_external_and_derived_models_cover_cpp_registrations(self):
        text = (INPUT / "properties/properties.xml").read_text()
        for category in ("external", "derived"):
            manager = (ROOT / f"cpp/properties/{category}/{category}_model_manager.cpp").read_text()
            names = set(re.findall(r'if \(name == "([^"]+)"\)', manager))
            for name in names:
                block = re.search(rf'<{category}_property name="{name}">(.*?)</{category}_property>', text, re.S)
                self.assertIsNotNone(block, name)
                source = (ROOT / f"cpp/properties/{category}/{name}/{name}_model_manager.cpp").read_text()
                models = re.findall(r'registry_\.register_model<.*>\("([^"]+)"', source)
                if "common_model_manager_.register_models()" in source or "register_common_models<" in source:
                    models += ["constant", "time_series"]
                for model in models:
                    with self.subTest(property=name, model=model):
                        self.assertIn(f'model="{model}"', block[1])

    def test_geometry_and_solver_options(self):
        geometry = (INPUT / "geometry_mesh/geometry_mesh.xml").read_text()
        for choice in ("ideal_multilayer_sphere", "stochastic_multilayer_particle", "hxt", "mmg3d", "del3d", "front3d", "gmsh", "netgen", "none"):
            self.assertIn(f"[{choice}]", geometry)
        for node in ("center", "outer_radius", "batch_mean_radius", "batch_radius_std",
                     "batch_mean_thickness", "batch_thickness_std", "within_particle_thickness_std",
                     "within_particle_radius_std", "unwrapping_coefficient", "target_sphericity",
                     "shape_expansion", "theta_min", "theta_max", "theta", "phi",
                     "angular_distance_min", "angular_distance_max", "space_sampler"):
            self.assertIn(f"<{node}", geometry)
        simulation = (INPUT / "simulation/simulation.xml").read_text()
        for choice in ("steady", "transient", "none", "jacobi", "symmetric_gauss_seidel", "single", "staggered"):
            self.assertIn(f"[{choice}]", simulation)
        for field in ("temperature", "concentration"):
            self.assertIn(field, simulation)
            self.assertIn(field, (INPUT / "postprocess/postprocess.xml").read_text())

    def test_physics_models_and_specialized_parameters(self):
        for physics in ("thermal", "radioactivity"):
            text = (INPUT / physics / f"{physics}.xml").read_text()
            for source in (ROOT / f"cpp/physics/{physics}").rglob("*.cpp"):
                if source.parent.name not in ("boundary", "source", "initial"):
                    continue
                for model in re.findall(r'model_name\s*(?:==|!=)\s*"([^"]+)"', source.read_text()):
                    with self.subTest(physics=physics, model=model):
                        self.assertIn(f'model="{model}"', text)
            self.assertIn('model="perfect_contact"', text)
        text = (INPUT / "material/material.xml").read_text()
        # 模板注释中的替代节点也必须包含模型需要的全部构造参数。
        for model, tag, parameters in (
            ("buffer.bison_2021", "conductivity", {"initial_mass_density"}),
            ("kernel.fink_2000_uo2", "specific_heat", {"u235_enrichment"}),
            ("common.arrhenius", "nuclide_diffusion_coef", {"D0", "Q"}),
        ):
            block = re.search(rf'<{tag} model="{model}">(.*?)</{tag}>', text, re.S)
            self.assertIsNotNone(block)
            self.assertEqual(set(re.findall(r'<parameter name="([^"]+)"', block[1])), parameters)


if __name__ == "__main__":
    unittest.main()
