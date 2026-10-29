from pathlib import Path

import numpy as np


def save_sample_csv(path: Path, data: np.ndarray, property_name: str) -> None:
    """保存编号、xyz、r、值; 保持原 CSV 格式, 分块写入以控制临时内存。"""
    row_format = "%d,%.18e,%.18e,%.18e,%.18e,%.18e\n"
    with path.open("w", encoding="utf-8") as stream:
        stream.write(f"index,x_m,y_m,z_m,r_m,{property_name}\n")
        for start in range(0, len(data), 8192):
            # tolist 在 NumPy 内批量转换数值, 避免逐行访问 NumPy 标量。
            rows = data[start:start + 8192].tolist()
            stream.write("".join(row_format % tuple(row) for row in rows))
