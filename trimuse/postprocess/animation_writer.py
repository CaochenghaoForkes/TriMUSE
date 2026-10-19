from pathlib import Path

from PIL import GifImagePlugin, Image


def save_gif(frame_paths: list[Path], path: Path, duration: int = 120) -> Path:
    """按时间顺序编码已有 PNG, 共用调色板; 逐帧写入, 不把整段动画加载到内存"""
    with Image.open(frame_paths[0]) as image:
        palette = image.convert("RGB").quantize(colors=256)
    try:
        with path.open("wb") as stream:
            header, _ = GifImagePlugin.getheader(palette, info={"loop": 0, "optimize": False})
            for block in header:
                stream.write(block)
            for frame_path in frame_paths:
                with Image.open(frame_path) as image:
                    frame = image.convert("RGB").quantize(palette=palette, dither=Image.Dither.NONE)
                try:
                    for block in GifImagePlugin.getdata(frame, duration=duration, disposal=2):
                        stream.write(block)
                finally:
                    frame.close()
            stream.write(b";")
    finally:
        palette.close()
    return path
