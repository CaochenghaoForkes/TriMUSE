"""公共时间步只配置求解顺序，不需要交换间隔。"""
from dataclasses import asdict
from pathlib import Path
import tempfile
import unittest
from xml.etree import ElementTree as ET

from trimuse.input.simulation.config import SimulationCardLoader
from trimuse.input.simulation.examine import SimulationCardExamine
from trimuse.output import OutputManager


class SimulationCouplingTests(unittest.TestCase):
    def test_read_check_and_snapshot_without_exchange_interval(self):
        root = ET.fromstring('''<simulation>
            <problem>
                <time_span value="1" unit="[s]"/>
                <min_time_step value="0.1" unit="[s]"/>
                <field name="temperature" model="transient"/>
                <field name="concentration" model="transient"/>
            </problem>
            <solver>
                <quadrature domain_order="4" boundary_order="3"/>
                <field name="temperature"/><field name="concentration"/>
            </solver>
            <coupling><step fields="temperature concentration" method="single"/></coupling>
            <output><write_interval value="0.1" unit="[s]"/></output>
        </simulation>''')
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "simulation.xml"
            ET.ElementTree(root).write(path)
            card = SimulationCardLoader.load(path)
            SimulationCardExamine.examine(card)
            self.assertEqual(card.coupling.step[0].fields, ["temperature", "concentration"])
            self.assertNotIn("exchange_interval", asdict(card.coupling))
            OutputManager._simulation_card(root, card)
            self.assertEqual(root.find("coupling").attrib, {})
            ET.ElementTree(root).write(path)
            restored = SimulationCardLoader.load(path)
            SimulationCardExamine.examine(restored)
            self.assertEqual(asdict(card.coupling), asdict(restored.coupling))


if __name__ == "__main__":
    unittest.main()
