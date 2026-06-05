/*
 * sd_assets.h
 * SD 卡资源接口：挂载 FAT32 SD 卡，通过根索引读取 IMG/ 下的图片动画，
 * 以及 TEXT/ 下的点阵字库资源。
 */
#ifndef SD_ASSETS_H
#define SD_ASSETS_H                                      /* 防止 sd_assets.h 被重复包含。 */

#include <stdint.h>

#define SD_ASSET_ROW_MAX_BYTES          32u             /* SD 图片单行缓存上限，覆盖 250 像素以内的 1bpp 行。 */
#define SD_ASSET_IMAGE_MASCOT           1u              /* IMG 目录资源 ID：S1 全屏 GIF。 */
#define SD_ASSET_IMAGE_HOURGLASS        2u              /* IMG 目录资源 ID：主界面沙漏动画。 */
#define SD_ASSET_GLYPH_ZHENG            0x90D1u         /* TEXT 目录字库码点：郑。 */

typedef struct {
    int16_t default_x;                                   /* 图片默认绘制 X 坐标。 */
    int16_t default_y;                                   /* 图片默认绘制 Y 坐标。 */
    uint16_t width;                                      /* 图片帧宽度，单位像素。 */
    uint16_t height;                                     /* 图片帧高度，单位像素。 */
    uint16_t stride;                                     /* 每行占用字节数，按 8 像素对齐。 */
    uint16_t frame_bytes;                                /* 单帧数据字节数。 */
    uint8_t default_scale;                               /* 默认整数缩放倍数。 */
    uint8_t frame_count;                                 /* 文件中包含的关键帧数量。 */
} SdImageInfo;

typedef struct {
    uint16_t width;                                      /* 字形宽度，单位像素。 */
    uint16_t height;                                     /* 字形高度，单位像素。 */
    uint16_t stride;                                     /* 每行占用字节数，按 8 像素对齐。 */
} SdGlyphInfo;

uint8_t sd_assets_reload(void);                          /* 重新挂载 SD 卡、读取根索引并扫描资源头。 */
uint8_t sd_assets_get_image_info(uint8_t image_id, SdImageInfo *info); /* 读取指定 IMG 图片资源的尺寸、帧数和默认位置。 */
uint8_t sd_assets_begin_image_frame(uint8_t image_id, uint8_t index); /* 将指定 IMG 图片资源读指针定位到某一帧。 */
uint8_t sd_assets_read_image_row(uint8_t *row, uint16_t row_bytes); /* 从当前图片帧顺序读取一行 1bpp 数据。 */
uint8_t sd_assets_begin_glyph(uint16_t codepoint, SdGlyphInfo *info); /* 在 TEXT 字库中定位指定码点字形。 */
uint8_t sd_assets_read_glyph_row(uint8_t *row, uint16_t row_bytes); /* 从当前字形顺序读取一行 1bpp 数据。 */
uint8_t sd_assets_write_probe(void);                     /* 向 SD 卡写入一条测试记录，用于验证 FAT32 写路径。 */
uint8_t sd_assets_last_error(void);                      /* 返回最近一次 SD/FAT32 资源操作错误码。 */

#endif
