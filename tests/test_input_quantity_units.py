"""输入只做数量单位换算，衰变常数不参与通用读取。"""
import math
import unittest
from xml.etree import ElementTree as ET

import numpy as np

from trimuse.input.units import Units
from trimuse.input.xml_utils import XmlUtils
from trimuse.input.properties.config import PropertiesCardLoader
from trimuse.input.radioactivity.config import RadioactivityCard, RadioactivityCardLoader
from trimuse.input.radioactivity.examine import RadioactivityCardExamine


class QuantityUnitTests(unittest.TestCase):
    avogadro = 6.02214076e23

    def test_quantity_and_compound_units(self):
        for source, target, factor in (
            ("[mol]", "[atom]", self.avogadro),
            ("[atom]", "[mol]", 1 / self.avogadro),
            ("[mmol]/[cm]^3", "[atom]/[m]^3", self.avogadro * 1000),
            ("[mol]/[s]", "[atom]/[s]", self.avogadro),
            ("[mol]/[m]^3[s]", "[atom]/[m]^3[s]", self.avogadro),
            ("[mol]/[cm]^2[s]", "[atom]/[m]^2[s]", self.avogadro * 10000),
            ("[mmol]", "[mol]", 1e-3),
            ("[J]/[mol][K]", "[J]/[mol][K]", 1),
        ):
            with self.subTest(source=source, target=target):
                self.assertTrue(math.isclose(Units.convert(2., source, target), 2 * factor, rel_tol=1e-14))
        np.testing.assert_allclose(Units.convert([0., 1., 2.], "[mol]", "[atom]"),
                                   np.array([0., 1., 2.]) * self.avogadro)

    def test_activity_cannot_convert_to_quantity(self):
        for activity in ("Bq", "kBq", "Ci", "mCi", "Hz"):
            for suffix in ("", "/[m]^3", "/[m]^2[s]"):
                with self.subTest(activity=activity, suffix=suffix):
                    with self.assertRaises(ValueError):
                        Units.convert(1., f"[{activity}]" + suffix, "[atom]" + suffix)
        with self.assertRaises(ValueError):
            Units.convert(1., "[atom]", "[Bq]")

    def test_standard_units_unchanged(self):
        self.assertAlmostEqual(Units.convert(20., "[C]", "[K]"), 293.15)
        self.assertAlmostEqual(Units.convert(1., "[um]", "[m]"), 1e-6)
        self.assertAlmostEqual(Units.convert(1., "[degree]", "[rad]"), math.pi / 180)

    def test_properties_do_not_need_nuclide_card(self):
        element = ET.fromstring('''<external_property name="nuclide_birth_rate">
            <global model="time_series">
                <parameter name="times" value="0 1" unit="[s]" si_unit="[s]"/>
                <parameter name="values" value="1 2" unit="[mol]/[s]" si_unit="[atom]/[s]"/>
            </global></external_property>''')
        config = PropertiesCardLoader.read_property(element)
        np.testing.assert_allclose(config.global_.parameters["values"],
                                   np.array([1., 2.]) * self.avogadro)

    def test_inventory_and_decay_are_independent(self):
        element = ET.fromstring('''<initial><concentration region="kernel" model="uniform_from_inventory">
            <parameter name="inventory" value="1" unit="[mol]" si_unit="[atom]"/>
            </concentration></initial>''')
        initial = RadioactivityCardLoader.read_initial(element)
        self.assertTrue(math.isclose(initial.concentration[0].parameters["inventory"], self.avogadro))
        card = RadioactivityCard(RadioactivityCard.Nuclide("Cs137", 10.), initial)
        RadioactivityCardExamine.examine(card)
        self.assertAlmostEqual(card.nuclide.decay_constant, math.log(2) / 10)
        for half_life in (0., -1., float('nan'), float('inf')):
            card.nuclide.half_life = half_life
            with self.assertRaises(ValueError):
                RadioactivityCardExamine.examine(card)

    def test_xml_activity_is_rejected(self):
        element = ET.fromstring('''<global model="constant">
            <parameter name="constant_value" value="1" unit="[Bq]" si_unit="[atom]"/>
            </global>''')
        with self.assertRaises(ValueError):
            XmlUtils.read_model(element)


if __name__ == "__main__":
    unittest.main()
