from .config import ChiefCard

class ChiefCardExamine:
    """总调度配置的基本检查, 必填项和路径读取由 loader 负责"""

    @staticmethod
    def examine(card: ChiefCard) -> None:
        """检查随机种子和结束阶段"""
        if card.random_seed < 0:
            raise ValueError("random seed 必须非负")
        if card.workflow.end_at not in ("mesh", "calculate", "postprocess"):
            raise ValueError("workflow.end_at 必须为 mesh、calculate 或 postprocess")
