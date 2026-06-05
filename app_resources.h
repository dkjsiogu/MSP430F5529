/*
 * app_resources.h
 * 应用资源门面：向主程序和渲染层提供图片、字库和文本资源接口，
 * 隐藏底层 SD/FatFs 资源索引实现。
 */
#ifndef APP_RESOURCES_H
#define APP_RESOURCES_H                                  /* 防止 app_resources.h 被重复包含。 */

#include <stdint.h>

#define APP_RESOURCE_ROW_MAX_BYTES       32u             /* 图片和字形单行缓存上限，单位字节。 */
#define APP_RESOURCE_IMAGE_MASCOT        1u              /* GIF 页面默认动图资源 ID。 */
#define APP_RESOURCE_IMAGE_HOURGLASS     2u              /* 主界面沙漏动画资源 ID。 */

typedef struct {
    int16_t default_x;                                   /* 图片默认绘制 X 坐标。 */
    int16_t default_y;                                   /* 图片默认绘制 Y 坐标。 */
    uint16_t width;                                      /* 图片帧宽度，单位像素。 */
    uint16_t height;                                     /* 图片帧高度，单位像素。 */
    uint16_t stride;                                     /* 每行占用字节数，按 8 像素对齐。 */
    uint16_t frame_bytes;                                /* 单帧数据字节数。 */
    uint8_t default_scale;                               /* 默认整数缩放倍数。 */
    uint8_t frame_count;                                 /* 文件中包含的关键帧数量。 */
} AppImageInfo;

typedef struct {
    uint16_t width;                                      /* 字形宽度，单位像素。 */
    uint16_t height;                                     /* 字形高度，单位像素。 */
    uint16_t stride;                                     /* 每行占用字节数，按 8 像素对齐。 */
} AppGlyphInfo;

void app_resources_init(void);                           /* 初始化应用资源系统并尝试加载 SD 根索引。 */
uint8_t app_resources_reload(void);                      /* 重新读取 SD 根索引和资源头，用于 SD 内容变化后刷新。 */
uint8_t app_resources_get_image_info(uint8_t image_id, AppImageInfo *info); /* 读取图片资源尺寸、帧数和默认位置。 */
uint8_t app_resources_begin_image_frame(uint8_t image_id, uint8_t index); /* 将图片资源读指针定位到指定帧。 */
uint8_t app_resources_read_image_row(uint8_t *row, uint16_t row_bytes); /* 顺序读取当前图片帧的一行 1bpp 数据。 */
uint8_t app_resources_begin_zheng_glyph(AppGlyphInfo *info); /* 定位主界面“郑”字 24x24 字形。 */
uint8_t app_resources_begin_text_glyph(uint16_t codepoint, AppGlyphInfo *info); /* 定位阅读页 16x16 字形。 */
uint8_t app_resources_read_glyph_row(uint8_t *row, uint16_t row_bytes); /* 顺序读取当前字形的一行 1bpp 数据。 */
uint8_t app_resources_begin_book(uint32_t offset, uint32_t *size); /* 从 BOOK.TXT 指定字节偏移开始准备顺序读取。 */
uint8_t app_resources_read_book_byte(uint8_t *value);     /* 从当前 BOOK.TXT 位置读取一个字节。 */
uint8_t app_resources_first_gif_image(void);              /* 返回第一个可用 GIF 动图资源 ID。 */
uint8_t app_resources_step_gif_image(uint8_t current_id, int8_t direction); /* 按方向切换可用 GIF 动图资源。 */
uint8_t app_resources_write_probe(void);                  /* 写入 SDTEST.TXT 探针，用于验证 SD 写路径。 */
uint8_t app_resources_last_error(void);                   /* 返回底层最近一次资源错误码。 */

#endif
