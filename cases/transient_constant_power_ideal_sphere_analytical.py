"""两区同物性理想球的瞬态导热级数解, 供 1D/轴对称 2D/3D 算例共用。

初始温度与表面温度均为 Ts, t=0 开启核心均匀恒定热源, 外层无热源。
两区必须使用相同的常数 k、rho、cp; 界面理想接触。
所有输入使用 SI 单位; r 是到球心的距离。无限级数截断为 terms 项。
"""

from pathlib import Path
import sys
import xml.etree.ElementTree as ET

import numpy as np


def read_line_temperature(csv_path: str | Path) -> tuple[np.ndarray, np.ndarray]:
    """读取 line CSV 的 r_m、temperature 列, 返回半径 (m) 和温度 (K) 数组。"""
    data = np.genfromtxt(Path(csv_path).expanduser(), delimiter=",", names=True, ndmin=1)
    return data["r_m"], data["temperature"]


def transient_temperature(
    radius: float | list[float] | np.ndarray,
    time: float,
    core_radius: float = 500e-6,
    outer_radius: float = 1000e-6,
    power: float = 0.1,
    conductivity: float = 4.0,
    mass_density: float = 10000.0,
    specific_heat: float = 500.0,
    surface_temperature: float = 293.15,
    terms: int = 2000,
) -> float | np.ndarray:
    radius = np.asarray(radius, dtype=float)
    heat_source = power / (4.0 * np.pi * core_radius ** 3 / 3.0)
    diffusivity = conductivity / (mass_density * specific_heat)
    eigenvalues = np.arange(1, terms + 1, dtype=float) * np.pi / outer_radius
    angles = eigenvalues * core_radius
    coefficients = 2.0 * heat_source / (conductivity * outer_radius) * (np.sin(angles) - angles * np.cos(angles)) / eigenvalues ** 3
    time_factors = -np.expm1(-diffusivity * eigenvalues ** 2 * time)
    spatial_modes = np.sinc(np.outer(eigenvalues, radius.ravel()) / np.pi)
    temperature = surface_temperature + ((coefficients * time_factors) @ spatial_modes).reshape(radius.shape)

    return float(temperature) if temperature.ndim == 0 else temperature


def compare_line_temperature(label: str, csv_path: str | Path, time: float,
                             show: bool = True, value_limits: tuple[float, float] | None = None,
                             **parameters):
    """比较指定帧; time 为该 CSV 对应的实际模拟时间 (s), 不从 Cycle 编号推算。"""
    import matplotlib.pyplot as plt

    radii, fem_temperature = read_line_temperature(csv_path)
    analytical_temperature = transient_temperature(radii, time, **parameters)
    plt.rcParams['font.size'] = 16
    plt.rcParams['font.family'] = "Times New Roman"
    figure, axes = plt.subplots(figsize=(4*1.5, 3*1.5))
    axes.plot(radii, fem_temperature, color="#003366", linewidth=2.0, label=label)
    axes.scatter(radii, analytical_temperature, marker="v", facecolors="red",edgecolors="black", linewidths=0.7, s=15, label="Analytical", zorder=3)
    axes.set_xlabel("r (m)", fontstyle="italic")
    axes.set_ylabel("Temperature (K)", fontstyle="italic")
    if value_limits is not None:
        axes.set_ylim(*value_limits)
    axes.text(0.97, 0.95, f"t = {time:.2e} s", transform=axes.transAxes,
              ha="right", va="top", fontstyle="italic")
    axes.ticklabel_format(axis="both", style="sci", scilimits=(0, 0), useOffset=False, useMathText=True)
    axes.grid(False)
    axes.legend(frameon=False)
    figure.tight_layout()
    if show:
        plt.show()
    return figure, axes


def compare_temperature_gif(output_directory: str | Path, line_name: str = "radial",
                            label: str = "FEM", duration: int = 120, **parameters) -> Path:
    """从 seed 输出根目录匹配各帧 CSV 与 PVD 时间, 保存对比 PNG 和 GIF。

    duration 为每帧播放时长 (ms), 与真实模拟时间间隔无关。
    结果位于 postprocess/analytical/lines/<line_name>/temperature/。
    """
    import matplotlib.pyplot as plt

    # 复用项目已有的逐帧 GIF 编码器, 无需把全部图像同时载入内存。
    project_directory = str(Path(__file__).resolve().parents[1])
    if project_directory not in sys.path:
        sys.path.insert(0, project_directory)
    from trimuse.postprocess.animation_writer import save_gif

    directory = Path(output_directory).expanduser().resolve()
    root = ET.parse(directory / "thermal/temperature/temperature.pvd").getroot()
    csv_directory = directory / "postprocess/lines" / line_name / "temperature"
    frames = sorted((float(item.get("timestep")), csv_directory / f"{Path(item.get('file')).parent.name}.csv")
                    for item in root.findall("./Collection/DataSet"))

    # 两类结果、全部时间共用一个纵轴范围, 避免动画自动缩放掩盖升温过程。
    lower, upper = np.inf, -np.inf
    for time, csv_path in frames:
        radii, fem_temperature = read_line_temperature(csv_path)
        analytical_temperature = transient_temperature(radii, time, **parameters)
        lower = min(lower, fem_temperature.min(), analytical_temperature.min())
        upper = max(upper, fem_temperature.max(), analytical_temperature.max())
    padding = 0.08 * (upper - lower) if upper > lower else 1.0
    value_limits = (lower - padding, upper + padding)

    destination = directory / "postprocess/analytical/lines" / line_name / "temperature"
    destination.mkdir(parents=True, exist_ok=True)
    frame_paths = []
    for time, csv_path in frames:
        figure, _ = compare_line_temperature(label, csv_path, time, show=False,
                                             value_limits=value_limits, **parameters)
        path = destination / f"{csv_path.stem}.png"
        figure.savefig(path, dpi=150)
        plt.close(figure)
        frame_paths.append(path)
    return save_gif(frame_paths, destination / "comparison.gif", duration=duration)


if __name__ == "__main__":
    
    for dim in ['1','2','3']:
        compare_temperature_gif(output_directory=f"/Users/caochenghao/课题内容/TriMUSE/cases/transient_constant_power_ideal_sphere_{dim}d/output_transient_constant_power_ideal_sphere_{dim}d/seed_202609134",line_name="radial",label=f"{dim}d",duration=120)
