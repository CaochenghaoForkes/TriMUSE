import math

from .config import RadioactivityCard


class RadioactivityCardExamine:
    """检查核素初值，保留原始位置名称"""

    @classmethod
    def examine(cls, card: RadioactivityCard) -> None:
        cls.check_models(card.regions.region, {'diffusion': {'fick'}, 'capacity': {'unit'}, 'decay': {'pur_decay'}, 'source': {'volumetric_nuclide_birth_rate'}}, "radioactivity.regions")
        cls.check_models(card.boundaries.boundary, {'nuclide_boundary': {'robin', 'dirichlet', 'neumann'}}, "radioactivity.boundaries")
        cls.check_models(card.interfaces.interface, {'interface_nuclide_transfer': {'perfect_contact'}}, "radioactivity.interfaces")
        for region in card.regions.region:
            terms = {model.name for model in region.model}
            if "diffusion" not in terms:
                raise ValueError(f"radioactivity.regions.{region.name}: 缺少扩散组装项 diffusion")
        if any(model.name == "decay" for region in card.regions.region for model in region.model):
            half_life = card.parameters.get("half_life")
            if not isinstance(half_life, float) or not math.isfinite(half_life) or half_life <= 0:
                raise ValueError("radioactivity.parameters.half_life: 启用衰变需要有限正半衰期，内部单位为 s")
        for item in card.initial.concentration:
            location = f"initial.concentration.{item.region}"
            if item.model != "uniform_from_inventory":
                raise ValueError(f"{location}: model 必须为 uniform_from_inventory")
            inventory = item.parameters.get("inventory")
            if not isinstance(inventory, float) or not math.isfinite(inventory) or inventory < 0:
                raise ValueError(f"{location}: inventory 必须为有限非负标量，内部单位为 atom")

    @staticmethod
    def check_models(locations, allowed: dict[str, set[str]], context: str) -> None:
        for location in locations:
            terms = set()
            for model in location.model:
                if model.model_name not in allowed.get(model.name, set()):
                    raise ValueError(f"{context}.{location.name}: 不支持的组装项和模型 {model.name}/{model.model_name}")
                if model.name in terms:
                    raise ValueError(f"{context}.{location.name}: 组装项 {model.name} 重复配置")
                terms.add(model.name)
