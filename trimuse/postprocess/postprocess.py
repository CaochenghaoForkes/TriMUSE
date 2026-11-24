from __future__ import annotations
from typing import TYPE_CHECKING

from .domain_postprocess import DomainPostprocessManager
from .scalar_postprocess import ScalarPostprocessManager

if TYPE_CHECKING:
    from trimuse.input.xml_loader import InputCards

class PostprocessManager:
    """按输入配置调度域属性与标量属性后处理"""

    def __init__(self, input_cards: InputCards) -> None:
        self.managers = []
        domain = input_cards.postprocess.domain_properties
        if domain.names and (not domain.extra_process or domain.points or domain.lines or domain.sections):
            self.managers.append(DomainPostprocessManager(input_cards))
        if input_cards.postprocess.scalar_properties.names or domain.names and domain.extra_process:
            self.managers.append(ScalarPostprocessManager(input_cards))

    def run(self) -> None:
        for manager in self.managers:
            manager.run()
