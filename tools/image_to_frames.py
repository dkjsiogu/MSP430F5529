#!/usr/bin/env python3
"""把图片、GIF 或多张图片转换为 MSP430 墨水屏可直接使用的 1bpp 关键帧 C 数据。"""

from __future__ import annotations

import argparse
import re
from collections import deque
from pathlib import Path
from typing import Iterable, List, Sequence, Tuple

try:
    from PIL import Image, ImageDraw, ImageFilter, ImageOps, ImageSequence as PilImageSequence
except Exception as exc:  # pragma: no cover - 运行环境缺依赖时给出明确错误
    raise SystemExit("需要安装 Pillow：python -m pip install pillow") from exc


BAYER4 = (
    (0, 8, 2, 10),
    (12, 4, 14, 6),
    (3, 11, 1, 9),
    (15, 7, 13, 5),
)

BAYER8 = (
    (0, 32, 8, 40, 2, 34, 10, 42),
    (48, 16, 56, 24, 50, 18, 58, 26),
    (12, 44, 4, 36, 14, 46, 6, 38),
    (60, 28, 52, 20, 62, 30, 54, 22),
    (3, 35, 11, 43, 1, 33, 9, 41),
    (51, 19, 59, 27, 49, 17, 57, 25),
    (15, 47, 7, 39, 13, 45, 5, 37),
    (63, 31, 55, 23, 61, 29, 53, 21),
)


def c_identifier(value: str) -> str:
    """把任意名字转换成 C 标识符。"""
    ident = re.sub(r"[^0-9A-Za-z_]", "_", value)
    if not ident or ident[0].isdigit():
        ident = "_" + ident
    return ident


def resize_frame(img: Image.Image, width: int | None, height: int | None) -> Image.Image:
    """按给定尺寸缩放图片，只给一个方向时保持原始宽高比。"""
    if width is None and height is None:
        return img.copy()
    src_w, src_h = img.size
    if width is None:
        width = max(1, int(round(src_w * (height / src_h))))
    if height is None:
        height = max(1, int(round(src_h * (width / src_w))))
    return img.resize((width, height), Image.Resampling.LANCZOS)


def alpha_composite_on_white(img: Image.Image) -> Image.Image:
    """把带透明通道的图片合成到白底，同时保留 alpha 供主体 mask 使用。"""
    rgba = img.convert("RGBA")
    alpha = rgba.getchannel("A")
    white = Image.new("RGBA", rgba.size, (255, 255, 255, 255))
    composited = Image.alpha_composite(white, rgba)
    composited.putalpha(alpha)
    return composited


def load_input_frames(paths: Sequence[Path], max_frames: int, frame_step: int) -> List[Image.Image]:
    """加载普通图片或 GIF，返回抽样后的帧列表。"""
    frames: List[Image.Image] = []
    for path in paths:
        with Image.open(path) as img:
            source = PilImageSequence.Iterator(img) if getattr(img, "is_animated", False) else [img]
            for index, frame in enumerate(source):
                if index % frame_step != 0:
                    continue
                frames.append(frame.convert("RGBA").copy())
                if max_frames and len(frames) >= max_frames:
                    return frames
    return frames


def draw_demo_hourglass(width: int, height: int, frame_count: int) -> List[Image.Image]:
    """生成默认沙漏演示帧，便于没有外部图片时也能验证资源链路。"""
    frames: List[Image.Image] = []
    usable = max(1, frame_count - 3)
    for frame_index in range(frame_count):
        img = Image.new("L", (width, height), 255)
        draw = ImageDraw.Draw(img)
        cx = width // 2
        cy = height // 2 - 3
        flip_phase = frame_index - usable

        if flip_phase >= 0:
            angle_shift = [-7, 0, 7][min(flip_phase, 2)]
            draw.line((cx - 15, cy - 24, cx + 15, cy - 24 + angle_shift), fill=0, width=2)
            draw.line((cx - 15, cy + 24 - angle_shift, cx + 15, cy + 24), fill=0, width=2)
            draw.line((cx - 15, cy - 24, cx, cy), fill=0)
            draw.line((cx + 15, cy - 24 + angle_shift, cx, cy), fill=0)
            draw.line((cx, cy, cx - 15, cy + 24 - angle_shift), fill=0)
            draw.line((cx, cy, cx + 15, cy + 24), fill=0)
            draw.rectangle((cx - 2, cy - 2, cx + 2, cy + 2), fill=0)
            draw.rectangle((cx - 8 + flip_phase * 4, cy + 7, cx - 5 + flip_phase * 4, cy + 10), fill=0)
        else:
            progress = frame_index / max(1, usable - 1)
            draw.rectangle((cx - 18, cy - 30, cx + 18, cy - 28), fill=0)
            draw.rectangle((cx - 12, cy - 26, cx + 12, cy - 25), fill=0)
            draw.rectangle((cx - 18, cy + 28, cx + 18, cy + 30), fill=0)
            draw.rectangle((cx - 12, cy + 25, cx + 12, cy + 26), fill=0)
            draw.line((cx - 14, cy - 24, cx - 2, cy - 1), fill=0)
            draw.line((cx + 14, cy - 24, cx + 2, cy - 1), fill=0)
            draw.line((cx - 2, cy + 1, cx - 14, cy + 24), fill=0)
            draw.line((cx + 2, cy + 1, cx + 14, cy + 24), fill=0)
            draw.rectangle((cx - 3, cy - 1, cx + 3, cy + 1), fill=0)

            top_rows = round((1.0 - progress) * 18)
            bottom_rows = round(progress * 18)
            for row in range(top_rows):
                y = cy - 4 - row
                w = 2 + (row * row + 18) // 38
                if (row + frame_index) & 3:
                    draw.line((cx - w, y, cx + w, y), fill=0)
                else:
                    draw.line((cx - w + 1, y, cx + w - 1, y), fill=0)
            for row in range(bottom_rows):
                y = cy + 23 - row
                remaining = bottom_rows - row
                w = 2 + (remaining * remaining) // 34
                if ((row + frame_index) & 3) == 2 and w > 3:
                    draw.line((cx - w + 1, y, cx + w - 1, y), fill=0)
                else:
                    draw.line((cx - w, y, cx + w, y), fill=0)
            if 0.02 < progress < 0.98:
                draw.rectangle((cx, cy - 1, cx, cy + 3), fill=0)
                draw.rectangle((cx, cy + 5 + (frame_index & 3) * 3, cx, cy + 6 + (frame_index & 3) * 3), fill=0)
                draw.point((cx - 4 + (frame_index & 3), cy + 9), fill=0)
                draw.point((cx + 3 - (frame_index & 1), cy + 18), fill=0)

            for px, py in ((cx - 8, cy - 17), (cx - 6, cy - 11), (cx + 7, cy + 12), (cx + 9, cy + 18)):
                draw.point((px, py), fill=0)

        frames.append(img.convert("RGBA"))
    return frames


def preprocess_mono_frame(img: Image.Image, auto_contrast: bool, sharpen: float) -> Image.Image:
    """对缩放后的图片做灰度增强，减少小尺寸转黑白时的糊边。"""
    gray = img.convert("L")
    if auto_contrast:
        gray = ImageOps.autocontrast(gray)
    if sharpen > 0:
        gray = gray.filter(ImageFilter.UnsharpMask(radius=1.0, percent=int(100 * sharpen), threshold=2))
    return gray


def dither_matrix(size: int) -> Tuple[Tuple[int, ...], ...]:
    """返回有序抖动矩阵，8x8 比 4x4 提供更细的 64 档灰度。"""
    return BAYER8 if size == 8 else BAYER4


def subject_seed_mask(rgb: Image.Image, mask_value: int, mask_saturation: int, mask_white: int) -> Image.Image:
    """按颜色和亮度估计主体种子区域，优先抓黄色、粉色和白色高光。"""
    hsv = rgb.convert("HSV")
    pixels = hsv.load()
    mask = Image.new("L", rgb.size, 0)
    mask_pixels = mask.load()

    for y in range(rgb.height):
        for x in range(rgb.width):
            _h, saturation, value = pixels[x, y]
            if (value > mask_value and saturation > mask_saturation) or value > mask_white:
                mask_pixels[x, y] = 255

    return mask


def subject_mask(
    rgb: Image.Image,
    mask_value: int,
    mask_saturation: int,
    mask_white: int,
    mask_grow: int,
    mask_shrink: int,
) -> Image.Image:
    """用边界洪水填充剥离外部背景，同时保留嘴巴、眼睛等主体内部暗部。"""
    seed = subject_seed_mask(rgb, mask_value, mask_saturation, mask_white)
    barrier = seed.filter(ImageFilter.MaxFilter(mask_grow))
    if mask_shrink > 1:
        barrier = barrier.filter(ImageFilter.MinFilter(mask_shrink))

    width, height = rgb.size
    barrier_pixels = barrier.load()
    background = bytearray(width * height)
    queue: deque[Tuple[int, int]] = deque()

    def add_background(x: int, y: int) -> None:
        offset = y * width + x
        if background[offset] or barrier_pixels[x, y] != 0:
            return
        background[offset] = 1
        queue.append((x, y))

    for x in range(width):
        add_background(x, 0)
        add_background(x, height - 1)
    for y in range(height):
        add_background(0, y)
        add_background(width - 1, y)

    while queue:
        x, y = queue.popleft()
        if x > 0:
            add_background(x - 1, y)
        if x + 1 < width:
            add_background(x + 1, y)
        if y > 0:
            add_background(x, y - 1)
        if y + 1 < height:
            add_background(x, y + 1)

    mask = Image.new("L", rgb.size, 0)
    mask_pixels = mask.load()
    for y in range(height):
        row = y * width
        for x in range(width):
            if background[row + x] == 0:
                mask_pixels[x, y] = 255

    return mask.filter(ImageFilter.MinFilter(3)).filter(ImageFilter.MaxFilter(3))


def prepare_subject_frame(
    frame: Image.Image,
    width: int | None,
    height: int | None,
    mask_value: int,
    mask_saturation: int,
    mask_white: int,
    mask_grow: int,
    mask_shrink: int,
) -> Image.Image:
    """在原始尺寸先剥离黑背景，再缩放，防止背景颜色混入主体边缘。"""
    rgb = frame.convert("RGB")
    mask = subject_mask(rgb, mask_value, mask_saturation, mask_white, mask_grow, mask_shrink)
    white = Image.new("RGB", rgb.size, (255, 255, 255))
    clean_rgb = Image.composite(rgb, white, mask)
    rgba = clean_rgb.convert("RGBA")
    rgba.putalpha(mask)
    return alpha_composite_on_white(resize_frame(rgba, width, height))


def prepare_frame_for_pack(
    frame: Image.Image,
    width: int | None,
    height: int | None,
    mono_mode: str,
    mask_value: int,
    mask_saturation: int,
    mask_white: int,
    mask_grow: int,
    mask_shrink: int,
) -> Image.Image:
    """按转换模式准备待打包帧，subject 模式会先处理透明主体。"""
    if mono_mode == "subject":
        return prepare_subject_frame(frame, width, height, mask_value, mask_saturation,
                                     mask_white, mask_grow, mask_shrink)
    return resize_frame(frame, width, height)


def subject_pixel_ink(
    hue: int,
    saturation: int,
    value: int,
    in_subject: bool,
    base_ink: float,
    shadow_ink: float,
    saturation_ink: float,
    min_ink: float,
    max_ink: float,
    white_ink: float,
    dark_ink: float,
) -> float:
    """把主体像素映射成墨量，1.0 表示纯黑，0.0 表示纯白。"""
    _ = hue
    if not in_subject:
        return 0.0

    if value > 210 and saturation < 55:
        return white_ink
    if value < 75:
        return dark_ink

    ink = base_ink + shadow_ink * (1.0 - value / 255.0) + saturation_ink * (saturation / 255.0)
    if ink < min_ink:
        ink = min_ink
    if ink > max_ink:
        ink = max_ink
    return ink


def pack_frame(
    img: Image.Image,
    threshold: int,
    invert: bool,
    mono_mode: str,
    edge_threshold: int,
    auto_contrast: bool,
    sharpen: float,
    gamma: float,
    dither_size: int,
    subject_base: float,
    subject_shadow: float,
    subject_saturation: float,
    subject_min: float,
    subject_max: float,
    subject_edge: float,
    subject_white: float,
    subject_dark: float,
    subject_mask_value: int,
    subject_mask_saturation: int,
    subject_mask_white: int,
    subject_mask_grow: int,
    subject_mask_shrink: int,
    subject_core: int,
    subject_alpha: int,
) -> Tuple[int, int, int, bytes]:
    """把图片打包成 1bpp，MSB 在左，bit=1 表示黑色。"""
    alpha_pixels = img.getchannel("A").load() if "A" in img.getbands() else None
    rgb = img.convert("RGB")
    gray = preprocess_mono_frame(img, auto_contrast, sharpen)
    width, height = gray.size
    stride = (width + 7) // 8
    out = bytearray(stride * height)
    pixels = gray.load()
    subject_pixels = None
    subject_core_pixels = None
    hsv_pixels = None
    edge_pixels = None

    if mono_mode == "edge":
        edge = ImageOps.autocontrast(gray).filter(ImageFilter.FIND_EDGES).filter(ImageFilter.MaxFilter(3))
        pixels = edge.load()
    elif mono_mode == "dither":
        dithered = gray.convert("1", dither=Image.Dither.FLOYDSTEINBERG).convert("L")
        pixels = dithered.load()
    elif mono_mode == "subject":
        if alpha_pixels is not None:
            subject = img.getchannel("A").point(lambda value: 255 if value >= subject_alpha else 0)
        else:
            subject = subject_mask(rgb, subject_mask_value, subject_mask_saturation,
                                   subject_mask_white, subject_mask_grow, subject_mask_shrink)
        subject_pixels = subject.load()
        subject_core_pixels = subject.filter(ImageFilter.MinFilter(subject_core)).load()
        hsv_pixels = rgb.convert("HSV").load()
        edge_pixels = ImageOps.autocontrast(gray).filter(ImageFilter.FIND_EDGES).load()
    matrix = dither_matrix(dither_size)
    matrix_size = len(matrix)
    matrix_levels = matrix_size * matrix_size

    for y in range(height):
        for x in range(width):
            if mono_mode == "subject":
                hue, saturation, value = hsv_pixels[x, y]
                in_subject = subject_pixels[x, y] != 0
                in_core = subject_core_pixels[x, y] != 0
                if value < 75 and in_subject and not in_core:
                    ink = 0.0
                else:
                    ink = subject_pixel_ink(
                        hue, saturation, value, in_subject,
                        subject_base, subject_shadow, subject_saturation,
                        subject_min, subject_max, subject_white, subject_dark,
                    )
                if edge_pixels[x, y] > edge_threshold and in_core:
                    ink = max(ink, subject_edge)
                ink = pow(max(0.0, min(1.0, ink)), gamma)
                black = ink > ((matrix[y % matrix_size][x % matrix_size] + 0.5) / matrix_levels)
            elif mono_mode == "light":
                black = pixels[x, y] > threshold
            elif mono_mode == "edge":
                black = pixels[x, y] > edge_threshold
            elif mono_mode == "dither":
                black = pixels[x, y] == 0
            else:
                black = pixels[x, y] < threshold
            if invert:
                black = not black
            if black:
                out[y * stride + (x // 8)] |= 0x80 >> (x & 7)
    return width, height, stride, bytes(out)


def bytes_to_c_rows(data: bytes, indent: str = "    ") -> Iterable[str]:
    """把字节数组格式化成 C 初始化列表。"""
    for offset in range(0, len(data), 12):
        chunk = data[offset : offset + 12]
        yield indent + ", ".join(f"0x{value:02X}u" for value in chunk) + ","


def write_c_file(
    output: Path,
    symbol: str,
    frames: Sequence[Image.Image],
    width: int | None,
    height: int | None,
    threshold: int,
    invert: bool,
    mono_mode: str,
    edge_threshold: int,
    auto_contrast: bool,
    sharpen: float,
    gamma: float,
    dither_size: int,
    subject_base: float,
    subject_shadow: float,
    subject_saturation: float,
    subject_min: float,
    subject_max: float,
    subject_edge: float,
    subject_white: float,
    subject_dark: float,
    subject_mask_value: int,
    subject_mask_saturation: int,
    subject_mask_white: int,
    subject_mask_grow: int,
    subject_mask_shrink: int,
    subject_core: int,
    subject_alpha: int,
    default_x: int,
    default_y: int,
    default_scale: int,
    append: bool,
) -> None:
    """生成单独的 C 资源文件，供工程直接编译进 Flash。"""
    symbol = c_identifier(symbol)
    prepared = [
        prepare_frame_for_pack(frame, width, height, mono_mode,
                               subject_mask_value, subject_mask_saturation, subject_mask_white,
                               subject_mask_grow, subject_mask_shrink)
        for frame in frames
    ]
    packed = [
        pack_frame(frame, threshold, invert,
                   mono_mode, edge_threshold, auto_contrast, sharpen, gamma,
                   dither_size, subject_base, subject_shadow, subject_saturation,
                   subject_min, subject_max, subject_edge, subject_white, subject_dark,
                   subject_mask_value, subject_mask_saturation, subject_mask_white,
                   subject_mask_grow, subject_mask_shrink, subject_core, subject_alpha)
        for frame in prepared
    ]
    header: List[str] = [
        "/*",
        f" * {output.name}",
        " * 由 tools/image_to_frames.py 自动生成的图片关键帧数据。",
        " * const 数据默认进入 MSP430 程序 Flash，渲染层按帧号读取并贴到帧缓冲。",
        " */",
        '#include "image_frames.h"',
        "",
    ]
    lines: List[str] = []
    for index, (_w, _h, _stride, data) in enumerate(packed):
        lines.append(f"static const uint8_t {symbol}_frame_{index}[] = {{")
        lines.extend(bytes_to_c_rows(data))
        lines.append("};")
        lines.append("")

    lines.append(f"static const ImageFrame {symbol}_frames[] = {{")
    for index, (w, h, stride, _data) in enumerate(packed):
        lines.append(f"    {{{w}u, {h}u, {stride}u, {symbol}_frame_{index}}},")
    lines.append("};")
    lines.append("")
    lines.append(f"const ImageSequence {symbol} = {{")
    lines.append(f"    {default_x},")
    lines.append(f"    {default_y},")
    lines.append(f"    {max(1, min(default_scale, 8))}u,")
    lines.append(f"    {len(packed)}u,")
    lines.append(f"    {symbol}_frames")
    lines.append("};")
    lines.append("")
    if append and output.exists():
        current = output.read_text(encoding="utf-8").rstrip()
        output.write_text(current + "\n\n" + "\n".join(lines), encoding="utf-8")
    else:
        output.write_text("\n".join(header + lines), encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="把图片/GIF 转成 MSP430 墨水屏 1bpp 关键帧 C 数据。")
    parser.add_argument("images", nargs="*", type=Path, help="输入图片路径；GIF 会按帧展开。")
    parser.add_argument("-o", "--output", type=Path, default=Path("image_frames.c"), help="输出 C 文件路径。")
    parser.add_argument("--symbol", default="g_hourglass_sequence", help="生成的 ImageSequence 符号名。")
    parser.add_argument("--width", type=int, help="输出帧宽度；只给宽度时保持比例。")
    parser.add_argument("--height", type=int, help="输出帧高度；只给高度时保持比例。")
    parser.add_argument("--threshold", type=int, default=180, help="灰度阈值，小于阈值视为黑色。")
    parser.add_argument("--mono-mode", choices=("threshold", "light", "edge", "dither", "subject"),
                        default="threshold", help="黑白转换模式：threshold 暗部为黑，light 亮部为黑，edge 边缘为黑，dither 误差扩散，subject 主体分割灰度抖动。")
    parser.add_argument("--edge-threshold", type=int, default=36, help="edge 模式的边缘阈值，越小线条越多。")
    parser.add_argument("--auto-contrast", action="store_true", help="转黑白前自动拉伸灰度对比度。")
    parser.add_argument("--sharpen", type=float, default=0.0, help="转黑白前的锐化强度，0 表示不锐化。")
    parser.add_argument("--gamma", type=float, default=0.85, help="subject 模式墨量伽马，越小整体越黑。")
    parser.add_argument("--dither-size", type=int, choices=(4, 8), default=8, help="subject 模式有序抖动矩阵尺寸，8 表示 64 档模拟灰度。")
    parser.add_argument("--subject-base", type=float, default=0.16, help="subject 模式主体基础墨量。")
    parser.add_argument("--subject-shadow", type=float, default=0.28, help="subject 模式暗部增加墨量。")
    parser.add_argument("--subject-saturation", type=float, default=0.22, help="subject 模式颜色饱和度增加墨量。")
    parser.add_argument("--subject-min", type=float, default=0.12, help="subject 模式主体最小墨量。")
    parser.add_argument("--subject-max", type=float, default=0.82, help="subject 模式主体最大墨量。")
    parser.add_argument("--subject-edge", type=float, default=0.90, help="subject 模式边缘保护墨量。")
    parser.add_argument("--subject-white", type=float, default=0.04, help="subject 模式白色高光墨量。")
    parser.add_argument("--subject-dark", type=float, default=0.92, help="subject 模式深色细节墨量。")
    parser.add_argument("--subject-mask-value", type=int, default=95, help="subject 模式主体 mask 的最低亮度。")
    parser.add_argument("--subject-mask-saturation", type=int, default=45, help="subject 模式主体 mask 的最低饱和度。")
    parser.add_argument("--subject-mask-white", type=int, default=170, help="subject 模式直接视为主体白色区域的亮度。")
    parser.add_argument("--subject-mask-grow", type=int, default=5, help="subject 模式主体种子膨胀尺寸，用于封闭外轮廓。")
    parser.add_argument("--subject-mask-shrink", type=int, default=3, help="subject 模式主体种子收缩尺寸，用于去掉零散背景点。")
    parser.add_argument("--subject-core", type=int, default=5, help="subject 模式内部核心收缩尺寸，边缘增强只作用于核心区域。")
    parser.add_argument("--subject-alpha", type=int, default=16, help="subject 模式缩放后 alpha 大于该值才视为主体。")
    parser.add_argument("--invert", action="store_true", help="反转黑白。")
    parser.add_argument("--max-frames", type=int, default=0, help="最多输出多少帧，0 表示不限。")
    parser.add_argument("--frame-step", type=int, default=1, help="GIF 或多帧输入的抽帧步进。")
    parser.add_argument("--x", type=int, default=0, help="资源记录的默认 X 坐标。")
    parser.add_argument("--y", type=int, default=0, help="资源记录的默认 Y 坐标。")
    parser.add_argument("--scale", type=int, default=1, help="资源记录的默认整数缩放倍数。")
    parser.add_argument("--demo-hourglass", action="store_true", help="生成内置沙漏演示帧，不读取输入图片。")
    parser.add_argument("--append", action="store_true", help="追加到已有 C 文件，用于把多个图片序列统一放进 image_frames.c。")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if args.frame_step <= 0:
        raise SystemExit("--frame-step 必须大于 0")
    if args.demo_hourglass:
        demo_w = args.width or 48
        demo_h = args.height or 66
        frames = draw_demo_hourglass(demo_w, demo_h, args.max_frames or 12)
        width = demo_w
        height = demo_h
    else:
        if not args.images:
            raise SystemExit("请提供图片路径，或者使用 --demo-hourglass 生成示例资源。")
        frames = load_input_frames(args.images, args.max_frames, args.frame_step)
        width = args.width
        height = args.height
    if not frames:
        raise SystemExit("没有可输出的图片帧。")
    write_c_file(
        args.output,
        args.symbol,
        frames,
        width,
        height,
        args.threshold,
        args.invert,
        args.mono_mode,
        args.edge_threshold,
        args.auto_contrast,
        args.sharpen,
        args.gamma,
        args.dither_size,
        args.subject_base,
        args.subject_shadow,
        args.subject_saturation,
        args.subject_min,
        args.subject_max,
        args.subject_edge,
        args.subject_white,
        args.subject_dark,
        args.subject_mask_value,
        args.subject_mask_saturation,
        args.subject_mask_white,
        args.subject_mask_grow,
        args.subject_mask_shrink,
        args.subject_core,
        args.subject_alpha,
        args.x,
        args.y,
        args.scale,
        args.append,
    )


if __name__ == "__main__":
    main()
