import math

from .config import MechanicsCard

class MechanicsCardExamine:
    """检查力学卡自身，几何引用和属性依赖由卡间检查负责"""

    BOUNDARY_PROPERTIES = {
        "from_surface_traction": "external.surface_traction",
        "from_surface_displacement": "external.surface_displacement",
        "from_surface_pressure": "external.surface_pressure",
    }

    @classmethod
    def examine(cls, card: MechanicsCard) -> None:
        temperature = card.thermal_expansion_reference_temperature
        if not math.isfinite(temperature) or temperature < 0:
            raise ValueError("热膨胀参考温度必须为不低于 0 K 的有限值")
        for items, location in ((card.boundary, "boundary"), (card.interfaces.interface, "interfaces")):
            names = [item.name for item in items]
            if len(names) != len(set(names)):
                raise ValueError(f"mechanics.{location}: 名称不能重复")
        for boundary in card.boundary:
            location = f"mechanics.boundary.{boundary.name}"
            if boundary.model not in cls.BOUNDARY_PROPERTIES:
                raise ValueError(f"{location}: model 必须为 [{', '.join(cls.BOUNDARY_PROPERTIES)}]")
            if boundary.parameters:
                raise ValueError(f"{location}: 边界数值应在属性卡中配置，不接受模型参数")
        for interface in card.interfaces.interface:
            location = f"mechanics.interfaces.{interface.name}"
            if interface.model != "perfect_contact":
                raise ValueError(f"{location}: model 必须为 perfect_contact")
            if interface.parameters:
                raise ValueError(f"{location}: perfect_contact 不需要模型参数")
        regions = [item.region for item in card.body_force]
        if len(regions) != len(set(regions)):
            raise ValueError("mechanics.body_forces: 区域不能重复")
        for item in card.body_force:
            location = f"mechanics.body_forces.{item.region}"
            if item.model != "gravity":
                raise ValueError(f"{location}: model 必须为 gravity")
            if set(item.parameters) != {"acceleration"}:
                raise ValueError(f"{location}: gravity 只接受 acceleration 参数")
            acceleration = item.parameters["acceleration"]
            if not isinstance(acceleration, list) or not acceleration or not all(math.isfinite(value) for value in acceleration):
                raise ValueError(f"{location}: acceleration 必须为非空的有限数值向量")
