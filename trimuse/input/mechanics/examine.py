from .config import MechanicsCard


class MechanicsCardExamine:
    """检查力学卡的组装项与模型名"""

    @classmethod
    def examine(cls, card: MechanicsCard) -> None:
        cls.check_models(card.regions.region, {'stiffness': {'linear_elastic'}, 'thermal_expansion': {'linear'}, 'source': {'gravity'}}, "mechanics.regions")
        cls.check_models(card.boundaries.boundary, {'mechanics_boundary': {'from_surface_traction', 'from_surface_displacement', 'from_surface_pressure'}}, "mechanics.boundaries")
        cls.check_models(card.interfaces.interface, {'interface_contact': {'perfect_contact'}, 'interface_pressure': {'single_face', 'double_face'}}, "mechanics.interfaces")

        for interface in card.interfaces.interface:
            for model in interface.model:
                if model.name == "interface_pressure" and model.model_name == "single_face":
                    if set(model.parameters) != {"region"} or not isinstance(model.parameters["region"], str):
                        raise ValueError(f"mechanics.interfaces.{interface.name}: single_face 需要 region 名称参数")
                elif model.parameters:
                    raise ValueError(f"mechanics.interfaces.{interface.name}: {model.model_name} 不接收参数")
        for region in card.regions.region:
            if not any(model.name == "stiffness" for model in region.model):
                raise ValueError(f"mechanics.regions.{region.name}: 缺少刚度组装项 stiffness")

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
