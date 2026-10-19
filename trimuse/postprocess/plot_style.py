from functools import wraps


def plot_style(method):
    """统一论文绘图风格, 仅在当前绘图期间生效, 不修改全局 Matplotlib 配置"""
    @wraps(method)
    def wrapped(*args, **kwargs):
        import matplotlib

        with matplotlib.rc_context({
            "font.family": "serif",
            "font.serif": ["Times New Roman", "DejaVu Serif"],
            "font.size": 16,
            "axes.labelsize": 16,
            "axes.linewidth": 1.0,
            "xtick.labelsize": 14,
            "ytick.labelsize": 14,
            "xtick.major.width": 1.0,
            "ytick.major.width": 1.0,
            "lines.linewidth": 2.0,
            "axes.prop_cycle": matplotlib.cycler(color=["#4C72B0", "#DD8452", "#55A868", "#C44E52"]),
            "mathtext.fontset": "stix",
            "figure.figsize": (8.0, 6.0),
            "figure.facecolor": "white",
            "axes.facecolor": "white",
            "savefig.facecolor": "white",
            "savefig.dpi": 300,
        }):
            return method(*args, **kwargs)
    return wrapped


def annotate_time(axes, time: float) -> None:
    """时间放在坐标轴内部右上偏中位置, 不使用标题"""
    axes.text(0.97, 0.90, f"t = {time:.8g} s", transform=axes.transAxes,
              ha="right", va="center", bbox={"facecolor": "white", "edgecolor": "none", "alpha": 0.8})
