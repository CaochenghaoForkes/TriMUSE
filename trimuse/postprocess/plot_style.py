from functools import wraps
from math import degrees

BOUNDARY_COLORS = ["#E64B35", "#00A087", "#3C5488", "#F39B7F", "#DC00FF", "#333333", "#7E6148", "#4DBBD5"]


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
            "axes.labelweight": "normal",
            "axes.grid": False,
            "legend.frameon": False,
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


def annotate_details(axes, lines: list[str], position: tuple[float, float] = (0.97, 0.94), align: str = "right", fontsize: float = 12) -> None:
    """逐行斜体注释, 默认 12 pt, 在框内顶对齐"""
    axes.text(*position, "\n".join(lines), transform=axes.transAxes, ha=align, va="top",
              fontsize=fontsize, fontstyle="italic", linespacing=1.25,
              bbox={"facecolor": "white", "edgecolor": "none", "alpha": 0.8})


def annotate_time(axes, time: float, position: tuple[float, float] = (0.97, 0.94), details: list[str] | None = None, fontsize: float = 12) -> None:
    coefficient, exponent = f"{time:.2e}".split("e")
    label = rf"$t = {coefficient}\times10^{{{int(exponent)}}}\ \mathit{{s}}$"
    annotate_details(axes, [label, *(details or [])], position, fontsize=fontsize)


def coordinate_labels(point: tuple[float, float, float]) -> list[str]:
    return [f"{axis} = {value:.4g} m" for axis, value in zip("xyz", point)]


def direction_labels(theta: float, phi: float | None = None) -> list[str]:
    labels = [f"θ = {degrees(theta):.4g}°"]
    if phi is not None:
        labels.append(f"φ = {degrees(phi):.4g}°")
    return labels


def property_label(name: str, metadata: dict[str, dict[str, str]]) -> str:
    unit = metadata.get(name, {}).get("unit")
    return f"{name} ({unit})" if unit else name


def scientific_axes(axes) -> None:
    """x、y 轴统一显示科学计数法, 不使用加法偏移量"""
    axes.ticklabel_format(axis="both", style="sci", scilimits=(0, 0), useOffset=False, useMathText=True)
