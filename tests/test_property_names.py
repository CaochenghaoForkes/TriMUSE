from pathlib import Path
import re
import sys
import tempfile
import unittest
from xml.etree import ElementTree as ET

from trimuse.input.material.config import MaterialCard, MaterialCardLoader
from trimuse.input.material.examine import MaterialCardExamine
from trimuse.input.properties.config import PropertiesCard, PropertiesCardLoader
from trimuse.input.properties.examine import PropertiesCardExamine
from trimuse.input.xml_loader import XmlLoader
from trimuse.output import OutputManager


ROOT = Path(__file__).resolve().parents[1]


class PropertyNameTests(unittest.TestCase):
    def test_properties_xml_short_names_become_full_names(self):
        element = ET.fromstring('''<external_property name="power">
            <region name="kernel" model="common.constant">
                <parameter name="constant_value" value="2" unit="[W]" si_unit="[W]"/>
            </region></external_property>''')
        prop = PropertiesCardLoader.read_property(element)
        self.assertEqual(prop.name, "external.power")
        self.assertEqual(prop.region[0].model, "common.constant")
        PropertiesCardExamine.examine(PropertiesCard(external_property=[prop]))

    def test_material_xml_names_become_full_names(self):
        element = ET.fromstring('''<porosity model="common.constant">
            <parameter name="constant_value" value="0.2" unit="[1]" si_unit="[1]"/>
        </porosity>''')
        prop = MaterialCardLoader.read_property(element, "general")
        self.assertEqual(prop.property_name, "material.general.porosity")
        MaterialCardExamine.check_property(prop, "buffer")

    def test_bare_or_unknown_property_models_are_rejected(self):
        for model in ("constant", "time_series", "made_up.constant"):
            with self.subTest(model=model), self.assertRaises(ValueError):
                prop = PropertiesCard.Property("external.power", region=[
                    PropertiesCard.Region("kernel", model, {"constant_value": 1.0})])
                PropertiesCardExamine.examine(PropertiesCard(external_property=[prop]))
        for name, model in (("derived.volumetric_power", "uniform_from_power"),
                            ("derived.volumetric_nuclide_birth_rate", "uniform_from_birth_rate")):
            with self.subTest(name=name), self.assertRaises(ValueError):
                prop = PropertiesCard.Property(name, region=[PropertiesCard.Region("kernel", model)])
                PropertiesCardExamine.examine(PropertiesCard(derived_property=[prop]))
        for model in ("constant", "time_series", "unknown.bison_2021"):
            with self.subTest(model=model), self.assertRaises(ValueError):
                MaterialCardExamine.check_property(MaterialCard.Property(
                    "material.thermal.conductivity", model, {"constant_value": 1.0}), "kernel")

    def test_common_and_builtin_material_models_are_accepted(self):
        for model in ("common.uniform_from_power", "common.uniform_from_birth_rate"):
            name = "derived.volumetric_power" if model.endswith("from_power") else "derived.volumetric_nuclide_birth_rate"
            prop = PropertiesCard.Property(name, region=[PropertiesCard.Region("kernel", model)])
            PropertiesCardExamine.examine(PropertiesCard(derived_property=[prop]))
        for model, parameters in (("kernel.bison_2021_uco", {}),
                                  ("buffer.bison_2021", {"initial_mass_density": 1000.0}),
                                  ("sic.miller", {})):
            MaterialCardExamine.check_property(MaterialCard.Property(
                "material.thermal.conductivity", model, parameters), "material")

    def test_snapshot_writes_short_xml_names_and_rereads_identically(self):
        prop = PropertiesCard.Property("external.power", region=[
            PropertiesCard.Region("kernel", "common.constant", {"constant_value": 2.0})])
        props = PropertiesCard(external_property=[prop])
        root = ET.fromstring('''<properties><external_property name="power">
            <region name="kernel" model="common.constant">
                <parameter name="constant_value" value="2" unit="[W]" si_unit="[W]"/>
            </region></external_property></properties>''')
        OutputManager.InputCardWriter._properties_card(root, props)
        self.assertEqual(root[0].get("name"), "power")
        self.assertEqual(PropertiesCardLoader.read_property(root[0]), prop)

        item = MaterialCard.Property("material.general.porosity", "common.constant", {"constant_value": 0.2})
        material = MaterialCard(mat=[MaterialCard.Mat("buffer", [item])], assign=MaterialCard.Assign([]))
        root = ET.fromstring('''<material><mat name="buffer"><general>
            <porosity model="common.constant">
                <parameter name="constant_value" value="0.2" unit="[1]" si_unit="[1]"/>
            </porosity></general></mat></material>''')
        OutputManager.InputCardWriter._material_card(root, material)
        self.assertIsNotNone(root.find("mat/general/porosity"))
        self.assertEqual(MaterialCardLoader.read_mat(root.find("mat")).properties, [item])

    def test_python_model_choices_match_cpp_registrations(self):
        choices = PropertiesCardExamine.MODELS | MaterialCardExamine.MODELS
        for name, expected in choices.items():
            with self.subTest(property=name):
                parts = name.split(".")
                folder = ROOT / "cpp/properties" / Path(*parts)
                source = (folder / f"{parts[-1]}_model_manager.cpp").read_text()
                actual = set(re.findall(r'register_model<[^>]+>\("([^"]+)"', source))
                self.assertEqual(actual, expected)
                self.assertTrue(all(re.fullmatch(r"(?:common|kernel|buffer|sic)\.[A-Za-z0-9_]+", model) for model in actual))
        source = (ROOT / "cpp/properties/properties_system.hpp").read_text()
        actual = set(re.findall(r'\{"([^"]+)", PropertyType::', source))
        self.assertEqual(actual, set(choices) | {"precomputed.region_volume", "external.time"})

    def test_xml_templates_use_registered_qualified_models(self):
        root = ET.parse(ROOT / "trimuse/input/properties/properties.xml").getroot()
        for prop in root:
            name = f'{prop.tag.removesuffix("_property")}.{prop.get("name")}'
            self.assertNotIn(".", prop.get("name"))
            for config in prop:
                self.assertIn(config.get("model"), PropertiesCardExamine.MODELS[name])
        root = ET.parse(ROOT / "trimuse/input/material/material.xml").getroot()
        for material in root.findall("mat"):
            for physics in material:
                for prop in physics:
                    name = f"material.{physics.tag}.{prop.tag}"
                    models = re.findall(r"\[([^\]]+)\]", prop.get("model")) or [prop.get("model")]
                    self.assertTrue(set(models) <= MaterialCardExamine.MODELS[name])

    def test_current_case_and_full_snapshot_reload(self):
        case = ROOT / "cases/transient_half_heat_2d/case.xml"
        if not case.exists():
            self.skipTest("local case not present")
        cards = XmlLoader.load(case)
        self.assertTrue(all(item.name.startswith("external.") for item in cards.properties.external_property))
        self.assertTrue(all(item.property_name.startswith("material.") for mat in cards.material.mat for item in mat.properties))
        with tempfile.TemporaryDirectory() as directory:
            writer = OutputManager.InputCardWriter(cards, Path(directory))
            writer.write_input_cards()
            snapshot = XmlLoader.load(Path(directory) / "input/chief.xml")
            self.assertEqual(snapshot.properties, cards.properties)
            self.assertEqual(snapshot.material, cards.material)

    def test_native_assignment_and_dependency_names(self):
        sys.path.insert(0, str(ROOT / "build/bindings"))
        try:
            from _core import FemSystem, PropertiesSystem
        except ImportError:
            self.skipTest("native extension has not been built")
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "line.mesh"
            path.write_text("""MFEM mesh v1.0
dimension
1
elements
1
3 1 0 1
boundary
2
1 0 0
2 0 1
vertices
2
1
0
1
""")
            fem = FemSystem(str(path), 4, 3)
            fem.add_field("temperature", 1)
            properties = PropertiesSystem(fem)
            properties.assign(3, "external.power.common.time_series", {"times": [0.0, 1.0], "values": [2.0, 4.0]})
            properties.assign(3, "derived.volumetric_power.common.uniform_from_power", {})
            properties.assign(3, "material.thermal.conductivity.kernel.bison_2021_uco", {})
            properties.prepare_dependencies()
            for time in (0.0, 0.5, 1.0):
                properties.update_external(time)
                properties.update_derived()
                properties.update_material()
                properties.update_field_dependent_materials("temperature")
                properties.accept_current_state()
            for name in ("external.power.constant", "external.power.wrong.constant", "derived.volumetric_power.uniform_from_power"):
                with self.subTest(model=name), self.assertRaises((ValueError, IndexError)):
                    properties.assign(3, name, {"constant_value": 1.0})


if __name__ == "__main__":
    unittest.main()
