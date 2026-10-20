"""两区理想球的稳态导热解析解, 供 1D/轴对称 2D/3D 算例共用。

核心均匀发热、外层无热源, 界面理想接触, 外表面为恒定温度。
所有输入使用 SI 单位; 这里的 r 是到球心的距离, 不是单独的 x 坐标。
"""

from pathlib import Path

import numpy as np


def read_line_temperature(csv_path: str | Path) -> tuple[np.ndarray, np.ndarray]:
    """读取 line CSV 的 r_m、temperature 列, 返回半径 (m) 和温度 (K) 数组。"""
    data = np.genfromtxt(Path(csv_path).expanduser(), delimiter=",", names=True, ndmin=1)
    return data["r_m"], data["temperature"]


def steady_temperature(
    radius: float | list[float] | np.ndarray,
    core_radius: float = 500e-6,
    outer_radius: float = 1000e-6,
    power: float = 0.1,
    core_conductivity: float = 4.0,
    shell_conductivity: float = 2.0,
    surface_temperature: float = 293.15,
) -> float | np.ndarray:
    radius = np.asarray(radius, dtype=float)
    temperature = np.empty_like(radius)
    core = radius <= core_radius

    interface_temperature = surface_temperature + power / (4.0 * np.pi * shell_conductivity) * (1.0 / core_radius - 1.0 / outer_radius)
    temperature[core] = interface_temperature + power / (8.0 * np.pi * core_conductivity * core_radius) * (1.0 - (radius[core] / core_radius) ** 2)
    temperature[~core] = surface_temperature + power / (4.0 * np.pi * shell_conductivity) * (1.0 / radius[~core] - 1.0 / outer_radius)

    return float(temperature) if temperature.ndim == 0 else temperature


def compare_line_temperature(label,csv_path: str | Path, **parameters: float):
    """读取有限元温度并与解析解绘图比较; parameters 转交 steady_temperature。"""
    import matplotlib.pyplot as plt

    radii, fem_temperature = read_line_temperature(csv_path)
    analytical_temperature = steady_temperature(radii, **parameters)
    plt.rcParams['font.size'] = 16
    plt.rcParams['font.family'] = "Times New Roman"
    figure, axes = plt.subplots(figsize=(4*1.5, 3*1.5))
    axes.plot(radii, fem_temperature, color="#003366", linewidth=2.0, label=label)
    axes.scatter(radii, analytical_temperature, marker="v", facecolors="red",edgecolors="black", linewidths=0.7, s=15, label="Analytical", zorder=3)
    axes.set_xlabel("r (m)", fontstyle="italic")
    axes.set_ylabel("Temperature (K)", fontstyle="italic")
    axes.ticklabel_format(axis="both", style="sci", scilimits=(0, 0), useOffset=False, useMathText=True)
    axes.grid(False)
    axes.legend(frameon=False)
    figure.tight_layout()
    plt.show()
    return figure, axes


if __name__ == "__main__":
    for dim in ['1','2','3']:
        compare_line_temperature(f'{dim}d',f'/Users/caochenghao/课题内容/TriMUSE/cases/steady_constant_power_ideal_sphere_{dim}d/output_steady_constant_power_ideal_sphere_{dim}d/seed_202609134/postprocess/lines/radial/temperature/Cycle000000.csv')
