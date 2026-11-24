import math

from .config import PostprocessCard

class PostprocessCardExamine:
    """检查后处理卡自身, 维数要求由卡间检查负责"""

    @classmethod
    def examine(cls, card: PostprocessCard) -> None:
        formats = card.output.formats
        if not formats or len(formats) != len(set(formats)) or set(formats) - {"png", "csv", "gif"}:
            raise ValueError("postprocess.output: formats 必须从 png、csv、gif 中选择, 非空且不能重复")
        if not card.domain_properties.names and not card.scalar_properties.names:
            raise ValueError("postprocess: 至少指定一项域属性或标量属性")
        for group in ("domain_properties", "scalar_properties"):
            names = getattr(card, group).names
            if len(names) != len(set(names)):
                raise ValueError(f"postprocess.{group}: names 不能重复")
        domain = card.domain_properties
        for group, items in (("points", domain.points), ("lines", domain.lines), ("sections", domain.sections)):
            names = [item.name for item in items]
            if len(names) != len(set(names)):
                raise ValueError(f"postprocess.domain_properties.{group}: 名称不能重复")
            for item in items:
                if group != "lines" and item.center is not None:
                    if any(value is not None and not math.isfinite(value) for value in (item.center.x, item.center.y, item.center.z)):
                        raise ValueError(f"postprocess.domain_properties.{group}.{item.name}: 坐标必须有限")
        for line in domain.lines:
            if line.sample_count < 2:
                raise ValueError(f"postprocess.domain_properties.lines.{line.name}: sample_count 必须至少为 2")
            cls.check_direction(line.direction, f"domain_properties.lines.{line.name}.direction")
        for section in domain.sections:
            cls.check_direction(section.normal, f"domain_properties.sections.{section.name}.normal")

    @staticmethod
    def check_direction(direction: PostprocessCard.Direction | None, location: str) -> None:
        if direction is None:
            return
        if not math.isfinite(direction.theta) or not 0 <= direction.theta <= math.pi:
            raise ValueError(f"postprocess.{location}: theta 必须在 [0,π] 内")
        if direction.phi is not None and (not math.isfinite(direction.phi) or not 0 <= direction.phi < 2 * math.pi):
            raise ValueError(f"postprocess.{location}: phi 必须在 [0,2π) 内")
