/*
 * image_frames.h
 * 墨水屏图片帧资源接口：定义由 Python 工具生成的一位图关键帧格式。
 * 这些 const 数据会随程序放入片内 Flash，渲染层只负责按帧号贴图。
 */
#ifndef IMAGE_FRAMES_H
#define IMAGE_FRAMES_H                                  /* 防止 image_frames.h 被重复包含。 */

#include "app_types.h"

typedef struct {
    uint16_t width;                                      /* 帧宽度，单位像素。 */
    uint16_t height;                                     /* 帧高度，单位像素。 */
    uint16_t stride;                                     /* 每行占用字节数，按 8 像素对齐。 */
    const uint8_t *data;                                 /* 1bpp 图像数据，bit=1 表示黑色像素。 */
} ImageFrame;

typedef struct {
    int16_t default_x;                                   /* 工具生成时记录的默认 X 坐标。 */
    int16_t default_y;                                   /* 工具生成时记录的默认 Y 坐标。 */
    uint8_t default_scale;                               /* 工具生成时记录的默认整数缩放倍数。 */
    uint8_t frame_count;                                 /* 序列包含的关键帧数量。 */
    const ImageFrame *frames;                            /* 关键帧描述数组。 */
} ImageSequence;

extern const ImageSequence g_hourglass_sequence;         /* 主界面沙漏动画关键帧序列。 */
extern const ImageSequence g_mascot_sequence;            /* S1 全屏播放使用的 GIF 转换动画帧序列。 */

#endif
