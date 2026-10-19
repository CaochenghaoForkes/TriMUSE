from .config import ChiefCard

class ChiefCardExamine:
    """总调度配置的基本检查, 必填项和路径读取由 loader 负责"""

    @staticmethod
    def examine(card: ChiefCard) -> None:
        """检查随机种子和执行阶段"""
        if card.random_seed < 0:
            raise ValueError("random seed 必须非负")
        if card.execution.target not in ("mesh", "solve", "postprocess"):
            raise ValueError("execution target 必须为 [mesh,solve,postprocess]")
