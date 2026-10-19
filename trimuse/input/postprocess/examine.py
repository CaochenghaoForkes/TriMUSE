import math

from .config import PostprocessCard

class PostprocessCardExamine:
    """检查后处理卡自身, 维数要求由卡间检查负责"""

    @classmethod
    def examine(cls, card: PostprocessCard) -> None:
        if not card.points and not card.lines and not card.sections:
            raise ValueError("postprocess: 至少配置一个 point、line 或 section")
        if not card.properties or len(card.properties) != len(set(card.properties)):
            raise ValueError("postprocess.properties: names 必须非空且不能重复")
        for group, items in (("points", card.points), ("lines", card.lines), ("sections", card.sections)):
            names = [item.name for item in items]
            if len(names) != len(set(names)):
                raise ValueError(f"postprocess.{group}: 名称不能重复")
            for item in items:
                if item.center is not None:
                    if any(value is not None and not math.isfinite(value) for value in (item.center.x, item.center.y, item.center.z)):
                        raise ValueError(f"postprocess.{group}.{item.name}: 坐标必须有限")
        for line in card.lines:
            if line.sample_count < 2:
                raise ValueError(f"postprocess.lines.{line.name}: sample_count 必须至少为 2")
            cls.check_direction(line.direction, f"lines.{line.name}.direction")
        for section in card.sections:
            cls.check_direction(section.normal, f"sections.{section.name}.normal")

    @staticmethod
    def check_direction(direction: PostprocessCard.Direction | None, location: str) -> None:
        if direction is None:
            return
        if not math.isfinite(direction.theta) or not 0 <= direction.theta <= math.pi:
            raise ValueError(f"postprocess.{location}: theta 必须在 [0,π] 内")
        if direction.phi is not None and (not math.isfinite(direction.phi) or not 0 <= direction.phi < 2 * math.pi):
            raise ValueError(f"postprocess.{location}: phi 必须在 [0,2π) 内")
