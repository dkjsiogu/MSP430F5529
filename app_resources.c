/*
 * app_resources.c
 * 应用资源门面实现：把 SD/FatFs 资源读取细节限制在本文件内，
 * 渲染层和主循环只依赖稳定的应用资源接口。
 */
#include "app_resources.h"

#include "sd_assets.h"

/* 把底层图片描述复制成应用层图片描述，避免上层直接依赖 sd_assets 类型。 */
static void app_resources_copy_image(const SdImageInfo *src, AppImageInfo *dst)
{
    dst->default_x = src->default_x;
    dst->default_y = src->default_y;
    dst->width = src->width;
    dst->height = src->height;
    dst->stride = src->stride;
    dst->frame_bytes = src->frame_bytes;
    dst->default_scale = src->default_scale;
    dst->frame_count = src->frame_count;
}

/* 把底层字形描述复制成应用层字形描述，避免上层直接依赖 sd_assets 类型。 */
static void app_resources_copy_glyph(const SdGlyphInfo *src, AppGlyphInfo *dst)
{
    dst->width = src->width;
    dst->height = src->height;
    dst->stride = src->stride;
}

void app_resources_init(void)
{
    (void)sd_assets_reload();
}

uint8_t app_resources_reload(void)
{
    return sd_assets_reload();
}

uint8_t app_resources_get_image_info(uint8_t image_id, AppImageInfo *info)
{
    SdImageInfo sd_info;

    if (info == 0 || !sd_assets_get_image_info(image_id, &sd_info)) {
        return 0;
    }
    app_resources_copy_image(&sd_info, info);
    return 1;
}

uint8_t app_resources_begin_image_frame(uint8_t image_id, uint8_t index)
{
    return sd_assets_begin_image_frame(image_id, index);
}

uint8_t app_resources_read_image_row(uint8_t *row, uint16_t row_bytes)
{
    return sd_assets_read_image_row(row, row_bytes);
}

uint8_t app_resources_begin_zheng_glyph(AppGlyphInfo *info)
{
    SdGlyphInfo sd_info;

    if (info == 0 || !sd_assets_begin_glyph(SD_ASSET_GLYPH_ZHENG, &sd_info)) {
        return 0;
    }
    app_resources_copy_glyph(&sd_info, info);
    return 1;
}

uint8_t app_resources_begin_text_glyph(uint16_t codepoint, AppGlyphInfo *info)
{
    SdGlyphInfo sd_info;

    if (info == 0 || !sd_assets_begin_text_glyph(codepoint, &sd_info)) {
        return 0;
    }
    app_resources_copy_glyph(&sd_info, info);
    return 1;
}

uint8_t app_resources_read_glyph_row(uint8_t *row, uint16_t row_bytes)
{
    return sd_assets_read_glyph_row(row, row_bytes);
}

uint8_t app_resources_begin_book(uint32_t offset, uint32_t *size)
{
    return sd_assets_begin_text(offset, size);
}

uint8_t app_resources_read_book_byte(uint8_t *value)
{
    return sd_assets_read_text_byte(value);
}

uint8_t app_resources_first_gif_image(void)
{
    return sd_assets_first_gif_image();
}

uint8_t app_resources_step_gif_image(uint8_t current_id, int8_t direction)
{
    return sd_assets_step_gif_image(current_id, direction);
}

uint8_t app_resources_write_probe(void)
{
    return sd_assets_write_probe();
}

uint8_t app_resources_last_error(void)
{
    return sd_assets_last_error();
}
