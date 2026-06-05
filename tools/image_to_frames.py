#!/usr/bin/env python3
"""把图片、GIF 或文字转换为 MSP430 墨水屏可用的 C 数据或 SD 二进制资源。"""

from __future__ import annotations

import argparse
import re
import struct
from collections import deque
from pathlib import Path
from typing import Iterable, List, Sequence, Tuple

try:
    from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageOps, ImageSequence as PilImageSequence
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

ASSET_INDEX_ENTRIES = (
    (1, 1, "IMG/MASCOT.BIN"),
    (1, 2, "IMG/HOURGLAS.BIN"),
    (2, 1, "TEXT/FONT24.BIN"),
    (2, 2, "TEXT/BOOK.TXT"),
    (2, 3, "TEXT/FONT16.BIN"),
)

OPTIONAL_IMAGE_ENTRIES = (
    (1, 3, "IMG/GIF2.BIN"),
    (1, 4, "IMG/GIF3.BIN"),
    (1, 5, "IMG/GIF4.BIN"),
)


def c_identifier(value: str) -> str:
    """把任意名字转换成 C 标识符。"""
    ident = re.sub(r"[^0-9A-Za-z_]", "_", value)
    if not ident or ident[0].isdigit():
        ident = "_" + ident
    return ident


def read_text_auto(path: Path, encoding: str | None = None) -> str:
    """读取文本文件，默认按 UTF-8、GB18030、GBK 依次尝试。"""
    data = path.read_bytes()
    encodings = [encoding] if encoding else ["utf-8-sig", "utf-8", "gb18030", "gbk"]
    for enc in encodings:
        if not enc:
            continue
        try:
            return data.decode(enc)
        except UnicodeDecodeError:
            continue
    return data.decode(encodings[-1] or "utf-8", errors="replace")


def write_utf8_text(input_path: Path, output_path: Path, encoding: str | None = None) -> None:
    """把外部小说文本转成固件统一读取的 UTF-8 BOOK.TXT。"""
    text = read_text_auto(input_path, encoding)
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(text, encoding="utf-8", newline="\n")


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


def pack_images(
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
) -> List[Tuple[int, int, int, bytes]]:
    """按统一算法准备并打包图片帧，供 C 数组和 SD 二进制资源共用。"""
    prepared = [
        prepare_frame_for_pack(frame, width, height, mono_mode,
                               subject_mask_value, subject_mask_saturation, subject_mask_white,
                               subject_mask_grow, subject_mask_shrink)
        for frame in frames
    ]
    return [
        pack_frame(frame, threshold, invert,
                   mono_mode, edge_threshold, auto_contrast, sharpen, gamma,
                   dither_size, subject_base, subject_shadow, subject_saturation,
                   subject_min, subject_max, subject_edge, subject_white, subject_dark,
                   subject_mask_value, subject_mask_saturation, subject_mask_white,
                   subject_mask_grow, subject_mask_shrink, subject_core, subject_alpha)
        for frame in prepared
    ]


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
    packed = pack_images(frames, width, height, threshold, invert, mono_mode,
                         edge_threshold, auto_contrast, sharpen, gamma, dither_size,
                         subject_base, subject_shadow, subject_saturation, subject_min,
                         subject_max, subject_edge, subject_white, subject_dark,
                         subject_mask_value, subject_mask_saturation, subject_mask_white,
                         subject_mask_grow, subject_mask_shrink, subject_core, subject_alpha)
    header: List[str] = [
        "/*",
        f" * {output.name}",
        " * 由 tools/image_to_frames.py 自动生成的图片关键帧数据。",
        " * const 数据默认进入 MSP430 程序 Flash，渲染层按帧号读取并贴到帧缓冲。",
        " */",
        '#include "image_types.h"',
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


def write_binary_file(
    output: Path,
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
) -> None:
    """生成可直接放到 FAT32 SD 卡根目录的二进制帧资源。"""
    packed = pack_images(frames, width, height, threshold, invert, mono_mode,
                         edge_threshold, auto_contrast, sharpen, gamma, dither_size,
                         subject_base, subject_shadow, subject_saturation, subject_min,
                         subject_max, subject_edge, subject_white, subject_dark,
                         subject_mask_value, subject_mask_saturation, subject_mask_white,
                         subject_mask_grow, subject_mask_shrink, subject_core, subject_alpha)
    if not packed:
        raise SystemExit("没有可写入 SD 资源文件的帧。")

    frame_w, frame_h, stride, first_data = packed[0]
    frame_bytes = stride * frame_h
    if frame_bytes > 0xFFFF:
        raise SystemExit("单帧数据超过 65535 字节，当前 MSP430 读取格式不支持。")
    if len(packed) > 255:
        raise SystemExit("帧数超过 255，当前 MSP430 读取格式不支持。")
    for w, h, row_stride, data in packed:
        if w != frame_w or h != frame_h or row_stride != stride or len(data) != frame_bytes:
            raise SystemExit("所有帧必须具有相同尺寸和 stride。")

    header = struct.pack(
        "<4sHHHHHBBhhHH",
        b"SIMG",
        1,
        24,
        frame_w,
        frame_h,
        stride,
        len(packed),
        max(1, min(default_scale, 8)),
        default_x,
        default_y,
        frame_bytes,
        0,
    )
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("wb") as f:
        f.write(header)
        f.write(first_data)
        for _w, _h, _stride, data in packed[1:]:
            f.write(data)


def draw_glyph_bitmap(char: str, font_path: Path, font_size: int, width: int, height: int,
                      threshold: int, x_offset: int, y_offset: int) -> bytes:
    """把一个字符渲染成固定尺寸 1bpp 点阵，bit=1 表示黑色。"""
    font = ImageFont.truetype(str(font_path), font_size)
    canvas = Image.new("L", (width, height), 255)
    draw = ImageDraw.Draw(canvas)
    bbox = draw.textbbox((0, 0), char, font=font)
    text_w = bbox[2] - bbox[0]
    text_h = bbox[3] - bbox[1]
    x = (width - text_w) // 2 - bbox[0] + x_offset
    y = (height - text_h) // 2 - bbox[1] + y_offset
    draw.text((x, y), char, font=font, fill=0)

    stride = (width + 7) // 8
    out = bytearray(stride * height)
    pixels = canvas.load()
    for py in range(height):
        for px in range(width):
            if pixels[px, py] < threshold:
                out[py * stride + (px // 8)] |= 0x80 >> (px & 7)
    return bytes(out)


def write_font_binary(output: Path, chars: str, font_path: Path, font_size: int,
                      width: int, height: int, threshold: int,
                      x_offset: int, y_offset: int, direct_index: bool) -> None:
    """生成 SD 卡 TEXT 字库资源，支持小字库 V2 和直接索引 V3 两种 SFNT 格式。"""
    if not chars:
        raise SystemExit("请通过 --font-chars 指定至少一个字。")
    if width <= 0 or height <= 0:
        raise SystemExit("字形宽高必须大于 0。")

    stride = (width + 7) // 8
    if stride > 32:
        raise SystemExit("字形单行超过 32 字节，当前 MSP430 行缓冲不支持。")
    glyphs = []
    seen = set()
    for char in sorted(chars, key=ord):
        codepoint = ord(char)
        if codepoint > 0xFFFF or codepoint < 32:
            continue
        if codepoint in seen:
            continue
        seen.add(codepoint)
        data = draw_glyph_bitmap(char, font_path, font_size, width, height,
                                 threshold, x_offset, y_offset)
        glyphs.append((codepoint, data))
    entry_size = 12
    if direct_index:
        header_size = 32
        lookup_offset = header_size
        entry_offset = lookup_offset + 65536 * 4
        data_offset = entry_offset + len(glyphs) * entry_size
    else:
        header_size = 16
        lookup_offset = 0
        entry_offset = header_size
        data_offset = entry_offset + len(glyphs) * entry_size
    entries = bytearray()
    payload = bytearray()
    lookup = bytearray(b"\xFF\xFF\xFF\xFF" * 65536) if direct_index else bytearray()
    current_entry_offset = entry_offset
    for codepoint, data in glyphs:
        if data_offset > 0xFFFFFFFF or len(data) > 0xFFFF:
            raise SystemExit("字库文件超过当前格式可表示范围。")
        entries += struct.pack("<HHIHH", codepoint, 0, data_offset, len(data), 0)
        if direct_index:
            lookup[codepoint * 4 : codepoint * 4 + 4] = struct.pack("<I", current_entry_offset)
        payload += data
        data_offset += len(data)
        current_entry_offset += entry_size

    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("wb") as f:
        if direct_index:
            f.write(struct.pack(
                "<4sHHHHHHHIIIH",
                b"SFNT",
                3,
                header_size,
                width,
                height,
                stride,
                len(glyphs),
                entry_size,
                lookup_offset,
                entry_offset,
                entry_offset + len(glyphs) * entry_size,
                0,
            ))
            f.write(lookup)
        else:
            f.write(struct.pack("<4sHHHHH", b"SFNT", 2, header_size, width, height, stride))
            f.write(struct.pack("<H", len(glyphs)))
        f.write(entries)
        f.write(payload)


def index_entries_for_root(root_dir: Path | None) -> list[tuple[int, int, str]]:
    """生成资源索引条目；扩展 GIF 文件存在时才写入索引。"""
    entries = list(ASSET_INDEX_ENTRIES)
    if root_dir is not None:
        for entry in OPTIONAL_IMAGE_ENTRIES:
            if (root_dir / entry[2]).exists():
                entries.append(entry)
    return entries


def write_asset_index(output: Path, root_dir: Path | None = None) -> None:
    """生成 SD 卡根目录 ASSET.IDX，固件按这个表定位 IMG 和 TEXT 资源。"""
    entries = index_entries_for_root(root_dir)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("wb") as f:
        f.write(struct.pack("<4sHH", b"AIDX", 1, len(entries)))
        for resource_type, resource_id, path in entries:
            encoded = path.encode("ascii")
            if len(encoded) >= 20:
                raise SystemExit(f"索引路径过长：{path}")
            f.write(struct.pack("<BBH", resource_type, resource_id, 0))
            f.write(encoded + b"\0" * (20 - len(encoded)))


def build_sd_layout(output_dir: Path) -> None:
    """创建固件约定的 SD 卡目录结构和根索引文件。"""
    (output_dir / "IMG").mkdir(parents=True, exist_ok=True)
    (output_dir / "TEXT").mkdir(parents=True, exist_ok=True)
    write_asset_index(output_dir / "ASSET.IDX", output_dir)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="把图片/GIF 转成 MSP430 墨水屏 1bpp 关键帧 C 数据或 SD 二进制资源。")
    parser.add_argument("images", nargs="*", type=Path, help="输入图片路径；GIF 会按帧展开。")
    parser.add_argument("-o", "--output", type=Path, help="输出 C 资源文件路径；当前工程默认使用 SD 二进制资源。")
    parser.add_argument("--binary-output", type=Path, help="输出 SD 卡二进制资源文件路径，例如 MASCOT.BIN。")
    parser.add_argument("--only-binary", action="store_true", help="只生成 SD 卡二进制资源，不改写 C 资源文件。")
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
    parser.add_argument("--append", action="store_true", help="追加到已有 C 文件，用于把多个图片序列统一放进同一个资源文件。")
    parser.add_argument("--font-output", type=Path, help="输出 SD 卡 SFNT 字库资源，例如 sdcard/TEXT/FONT24.BIN。")
    parser.add_argument("--font-chars", default="", help="要打包进 SFNT 字库的字符，例如 郑。")
    parser.add_argument("--font-chars-file", type=Path, help="从文本文件中收集需要打包进字库的字符。")
    parser.add_argument("--font-path", type=Path, default=Path(r"C:\Windows\Fonts\msyh.ttc"), help="用于生成点阵的 TrueType/OpenType 字体路径。")
    parser.add_argument("--font-size", type=int, default=25, help="字库生成使用的字体字号。")
    parser.add_argument("--font-width", type=int, default=24, help="字形输出宽度。")
    parser.add_argument("--font-height", type=int, default=24, help="字形输出高度。")
    parser.add_argument("--font-threshold", type=int, default=150, help="字形灰度阈值，小于阈值视为黑色。")
    parser.add_argument("--font-x-offset", type=int, default=0, help="字形水平微调偏移。")
    parser.add_argument("--font-y-offset", type=int, default=0, help="字形垂直微调偏移。")
    parser.add_argument("--font-direct-index", action="store_true", help="为大字库生成 65536 项直接索引表，提高 MSP430 查字速度。")
    parser.add_argument("--asset-index-output", type=Path, help="输出 SD 卡 ASSET.IDX 根索引。")
    parser.add_argument("--prepare-sd-layout", type=Path, help="创建 SD 卡资源目录并写入 ASSET.IDX。")
    parser.add_argument("--text-input", type=Path, help="外部小说文本路径，会自动识别常见中文编码。")
    parser.add_argument("--text-output", type=Path, help="输出固件读取的 UTF-8 文本，例如 sdcard/TEXT/BOOK.TXT。")
    parser.add_argument("--text-encoding", help="外部小说文本的编码；不指定时自动尝试 UTF-8/GB18030/GBK。")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    did_output = False
    if args.prepare_sd_layout:
        build_sd_layout(args.prepare_sd_layout)
        did_output = True
    if args.asset_index_output:
        write_asset_index(args.asset_index_output)
        did_output = True
    if args.text_input and args.text_output:
        write_utf8_text(args.text_input, args.text_output, args.text_encoding)
        did_output = True
    if args.font_output:
        font_chars = args.font_chars
        if args.font_chars_file:
            font_chars += read_text_auto(args.font_chars_file, "utf-8")
        write_font_binary(
            args.font_output,
            font_chars,
            args.font_path,
            args.font_size,
            args.font_width,
            args.font_height,
            args.font_threshold,
            args.font_x_offset,
            args.font_y_offset,
            args.font_direct_index,
        )
        did_output = True
    if did_output and not (args.images or args.demo_hourglass or args.binary_output or not args.only_binary):
        return
    if did_output and not args.images and not args.demo_hourglass and not args.binary_output:
        return
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
    if args.binary_output:
        write_binary_file(
            args.binary_output,
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
        )

    if args.output and not args.only_binary:
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
