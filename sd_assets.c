/*
 * sd_assets.c
 * FAT32 SD 卡资源读取层：通过 ASSET.IDX 找到 IMG 和 TEXT 目录中的资源。
 * 图片、字形和文本都按需流式读取，避免在 RAM 中保存整张图、整套字库或整本书。
 */
#include "sd_assets.h"

#include "fatfs/ff.h"

#define SD_ASSET_INDEX_NAME         "ASSET.IDX"      /* 根目录资源索引文件名。 */
#define SD_ASSET_PROBE_NAME         "SDTEST.TXT"     /* FAT32 写入探针文件名。 */
#define SD_ASSET_DEFAULT_MASCOT     "IMG/MASCOT.BIN" /* S1 全屏 GIF 默认路径。 */
#define SD_ASSET_DEFAULT_HOURGLASS  "IMG/HOURGLAS.BIN" /* 主界面沙漏默认路径，文件名保持 8.3。 */
#define SD_ASSET_DEFAULT_FONT24     "TEXT/FONT24.BIN" /* 24x24 点阵字库默认路径。 */
#define SD_ASSET_DEFAULT_BOOK       "TEXT/BOOK.TXT"  /* 阅读页 UTF-8 文本默认路径。 */
#define SD_ASSET_DEFAULT_FONT16     "TEXT/FONT16.BIN" /* 16x16 阅读页点阵字库默认路径。 */

#define SD_ASSET_PATH_LEN           20u              /* 索引中保存的 8.3 路径最大长度，含结束符。 */
#define SD_ASSET_IMAGE_SLOT_COUNT   5u               /* IMG 图片资源槽数量，包含主 GIF、沙漏和扩展 GIF。 */
#define SD_ASSET_FONT_SLOT_COUNT    2u               /* TEXT 字库资源槽数量，包含 24 点阵和 16 点阵。 */
#define SD_ASSET_INDEX_HEADER_SIZE  8u               /* ASSET.IDX 文件头长度。 */
#define SD_ASSET_INDEX_ENTRY_SIZE   24u              /* ASSET.IDX 单条索引长度。 */
#define SD_ASSET_INDEX_VERSION      1u               /* ASSET.IDX 格式版本。 */
#define SD_ASSET_INDEX_MAGIC_0      'A'              /* ASSET.IDX 魔数第 0 字节。 */
#define SD_ASSET_INDEX_MAGIC_1      'I'              /* ASSET.IDX 魔数第 1 字节。 */
#define SD_ASSET_INDEX_MAGIC_2      'D'              /* ASSET.IDX 魔数第 2 字节。 */
#define SD_ASSET_INDEX_MAGIC_3      'X'              /* ASSET.IDX 魔数第 3 字节。 */
#define SD_ASSET_TYPE_IMAGE         1u               /* ASSET.IDX 类型：图片资源。 */
#define SD_ASSET_TYPE_TEXT          2u               /* ASSET.IDX 类型：文字资源。 */

#define SD_IMAGE_MAGIC_0            'S'              /* SIMG 图片文件魔数第 0 字节。 */
#define SD_IMAGE_MAGIC_1            'I'              /* SIMG 图片文件魔数第 1 字节。 */
#define SD_IMAGE_MAGIC_2            'M'              /* SIMG 图片文件魔数第 2 字节。 */
#define SD_IMAGE_MAGIC_3            'G'              /* SIMG 图片文件魔数第 3 字节。 */
#define SD_IMAGE_VERSION            1u               /* SIMG 图片资源格式版本。 */
#define SD_IMAGE_HEADER_SIZE        24u              /* SIMG 图片资源文件头长度。 */

#define SD_FONT_MAGIC_0             'S'              /* SFNT 字库文件魔数第 0 字节。 */
#define SD_FONT_MAGIC_1             'F'              /* SFNT 字库文件魔数第 1 字节。 */
#define SD_FONT_MAGIC_2             'N'              /* SFNT 字库文件魔数第 2 字节。 */
#define SD_FONT_MAGIC_3             'T'              /* SFNT 字库文件魔数第 3 字节。 */
#define SD_FONT_VERSION_V1          1u               /* SFNT V1：旧 16 位偏移格式。 */
#define SD_FONT_VERSION_V2          2u               /* SFNT V2：32 位偏移格式，支持大字库。 */
#define SD_FONT_VERSION_V3          3u               /* SFNT V3：带 Unicode 直接索引表，优化阅读页查字速度。 */
#define SD_FONT_HEADER_SIZE         16u              /* SFNT V1/V2 字库文件头长度。 */
#define SD_FONT_HEADER_SIZE_V3      32u              /* SFNT V3 字库文件头长度。 */
#define SD_FONT_HEADER_READ_SIZE    32u              /* 读取字库头时使用的最大头长度。 */
#define SD_FONT_ENTRY_SIZE_V1       8u               /* SFNT V1 单个字形索引长度。 */
#define SD_FONT_ENTRY_SIZE_V2       12u              /* SFNT V2 单个字形索引长度。 */
#define SD_FONT_DIRECT_MISSING      0xFFFFFFFFUL     /* SFNT V3 直接索引表中的缺字标记。 */

#define SD_ASSET_ERR_NONE           0u               /* 最近一次操作成功。 */
#define SD_ASSET_ERR_MOUNT          1u               /* FAT32 挂载失败。 */
#define SD_ASSET_ERR_OPEN           2u               /* 打开资源文件失败。 */
#define SD_ASSET_ERR_HEADER         3u               /* 资源文件头读取或校验失败。 */
#define SD_ASSET_ERR_SIZE           4u               /* 资源行宽超过内部缓存。 */
#define SD_ASSET_ERR_READ           5u               /* 读取资源数据失败。 */
#define SD_ASSET_ERR_WRITE          6u               /* 写入探针文件失败。 */

#pragma DATA_SECTION(g_sd_fs, ".bss:usbram")
static FATFS g_sd_fs;
#pragma DATA_SECTION(g_asset_file, ".bss:usbram")
static FIL g_asset_file;
#pragma DATA_SECTION(g_image_info, ".bss:usbram")
static SdImageInfo g_image_info[SD_ASSET_IMAGE_SLOT_COUNT];
static uint16_t g_font_width[SD_ASSET_FONT_SLOT_COUNT];
static uint16_t g_font_height[SD_ASSET_FONT_SLOT_COUNT];
static uint16_t g_font_stride[SD_ASSET_FONT_SLOT_COUNT];
static uint16_t g_font_count[SD_ASSET_FONT_SLOT_COUNT];
static uint16_t g_font_entry_size[SD_ASSET_FONT_SLOT_COUNT];
static uint16_t g_font_header_size[SD_ASSET_FONT_SLOT_COUNT];
#pragma DATA_SECTION(g_font_lookup_offset, ".bss:usbram")
static uint32_t g_font_lookup_offset[SD_ASSET_FONT_SLOT_COUNT];
static uint8_t g_font_version[SD_ASSET_FONT_SLOT_COUNT];
static uint8_t g_sd_mounted = 0;
static uint8_t g_assets_loaded = 0;
static uint8_t g_asset_open = 0;
static uint8_t g_asset_open_type = 0;
static uint8_t g_asset_open_id = 0;
#pragma DATA_SECTION(g_image_ready, ".bss:usbram")
static uint8_t g_image_ready[SD_ASSET_IMAGE_SLOT_COUNT];
static uint8_t g_font_ready[SD_ASSET_FONT_SLOT_COUNT];
static uint8_t g_last_error = SD_ASSET_ERR_NONE;
#pragma DATA_SECTION(g_image_path, ".bss:usbram")
static char g_image_path[SD_ASSET_IMAGE_SLOT_COUNT][SD_ASSET_PATH_LEN];
#pragma DATA_SECTION(g_font24_path, ".bss:usbram")
static char g_font24_path[SD_ASSET_PATH_LEN];
#pragma DATA_SECTION(g_book_path, ".bss:usbram")
static char g_book_path[SD_ASSET_PATH_LEN];
#pragma DATA_SECTION(g_font16_path, ".bss:usbram")
static char g_font16_path[SD_ASSET_PATH_LEN];

static const uint8_t g_gif_image_ids[] = {
    SD_ASSET_IMAGE_MASCOT,
    SD_ASSET_IMAGE_GIF2,
    SD_ASSET_IMAGE_GIF3,
    SD_ASSET_IMAGE_GIF4
};

/* 从小端字节流读取 16 位无符号数。 */
static uint16_t sd_u16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

/* 从小端字节流读取 32 位无符号数。 */
static uint32_t sd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

/* 从小端字节流读取 16 位有符号数。 */
static int16_t sd_i16(const uint8_t *p)
{
    return (int16_t)sd_u16(p);
}

/* 复制 8.3 资源路径字面量，保证目标缓冲以 0 结尾。 */
static void sd_copy_literal(char *dst, const char *src)
{
    uint8_t i;

    for (i = 0; i < (SD_ASSET_PATH_LEN - 1u); i++) {
        dst[i] = src[i];
        if (src[i] == 0) {
            break;
        }
    }
    dst[SD_ASSET_PATH_LEN - 1u] = 0;
}

/* 从索引项复制路径，保证以 0 结尾。 */
static void sd_copy_index_path(char *dst, const uint8_t *src)
{
    uint8_t i;

    for (i = 0; i < (SD_ASSET_PATH_LEN - 1u); i++) {
        dst[i] = (char)src[i];
        if (src[i] == 0) {
            break;
        }
    }
    dst[SD_ASSET_PATH_LEN - 1u] = 0;
}

/* 按资源 ID 转换成图片槽位。 */
static uint8_t sd_image_slot(uint8_t image_id)
{
    if (image_id >= SD_ASSET_IMAGE_MASCOT &&
        image_id <= SD_ASSET_IMAGE_GIF4) {
        return (uint8_t)(image_id - 1u);
    }
    return 0xFFu;
}

/* 按资源 ID 转换成字库槽位。 */
static uint8_t sd_font_slot(uint8_t font_id)
{
    if (font_id == SD_ASSET_TEXT_FONT24) {
        return 0;
    }
    if (font_id == SD_ASSET_TEXT_FONT16) {
        return 1;
    }
    return 0xFFu;
}

/* 返回指定字库资源的路径缓冲。 */
static char *sd_font_path(uint8_t font_id)
{
    if (font_id == SD_ASSET_TEXT_FONT24) {
        return g_font24_path;
    }
    if (font_id == SD_ASSET_TEXT_FONT16) {
        return g_font16_path;
    }
    return 0;
}

/* 把路径缓冲恢复成标准目录结构，索引缺失时仍可按约定路径读取。 */
static void sd_assets_reset_paths(void)
{
    uint8_t i;

    for (i = 0; i < SD_ASSET_IMAGE_SLOT_COUNT; i++) {
        g_image_path[i][0] = 0;
    }
    sd_copy_literal(g_image_path[0], SD_ASSET_DEFAULT_MASCOT);
    sd_copy_literal(g_image_path[1], SD_ASSET_DEFAULT_HOURGLASS);
    sd_copy_literal(g_font24_path, SD_ASSET_DEFAULT_FONT24);
    sd_copy_literal(g_book_path, SD_ASSET_DEFAULT_BOOK);
    sd_copy_literal(g_font16_path, SD_ASSET_DEFAULT_FONT16);
}

/* 确保 FatFs 已经挂载，支持 32GB SDHC 卡由底层 CMD8/ACMD41/CMD58 初始化。 */
static uint8_t sd_assets_mount(void)
{
    if (g_sd_mounted) {
        return 1;
    }
    if (f_mount(0, &g_sd_fs) != FR_OK) {
        g_last_error = SD_ASSET_ERR_MOUNT;
        return 0;
    }
    g_sd_mounted = 1;
    return 1;
}

/* 关闭当前流式读取的资源文件。 */
static void sd_assets_close_file(void)
{
    if (g_asset_open) {
        (void)f_close(&g_asset_file);
    }
    g_asset_open = 0;
    g_asset_open_type = 0;
    g_asset_open_id = 0;
}

/* 打开指定资源文件；如果当前已经打开同一资源，则复用文件对象。 */
static uint8_t sd_assets_open_file(const char *path, uint8_t type, uint8_t id)
{
    if (path == 0 || path[0] == 0) {
        g_last_error = SD_ASSET_ERR_OPEN;
        return 0;
    }
    if (g_asset_open && g_asset_open_type == type && g_asset_open_id == id) {
        return 1;
    }

    sd_assets_close_file();
    if (f_open(&g_asset_file, path, FA_READ | FA_OPEN_EXISTING) != FR_OK) {
        g_last_error = SD_ASSET_ERR_OPEN;
        return 0;
    }
    g_asset_open = 1;
    g_asset_open_type = type;
    g_asset_open_id = id;
    return 1;
}

/* 校验并解析工具生成的 SIMG 图片资源文件头。 */
static uint8_t sd_assets_parse_image_header(const uint8_t *header, SdImageInfo *info)
{
    info->width = sd_u16(&header[8]);
    info->height = sd_u16(&header[10]);
    info->stride = sd_u16(&header[12]);
    info->frame_count = header[14];
    info->default_scale = header[15];
    info->default_x = sd_i16(&header[16]);
    info->default_y = sd_i16(&header[18]);
    info->frame_bytes = sd_u16(&header[20]);

    if (header[0] != SD_IMAGE_MAGIC_0 || header[1] != SD_IMAGE_MAGIC_1 ||
        header[2] != SD_IMAGE_MAGIC_2 || header[3] != SD_IMAGE_MAGIC_3) {
        return 0;
    }
    if (sd_u16(&header[4]) != SD_IMAGE_VERSION ||
        sd_u16(&header[6]) != SD_IMAGE_HEADER_SIZE) {
        return 0;
    }
    if (info->width == 0 || info->height == 0 ||
        info->stride == 0 || info->frame_count == 0) {
        return 0;
    }
    if (info->width > (uint16_t)(info->stride * 8u)) {
        return 0;
    }
    if (info->frame_bytes != (uint16_t)(info->stride * info->height)) {
        return 0;
    }
    if (info->stride > SD_ASSET_ROW_MAX_BYTES) {
        g_last_error = SD_ASSET_ERR_SIZE;
        return 0;
    }
    if (info->default_scale == 0) {
        info->default_scale = 1;
    }
    return 1;
}

/* 读取某个图片资源的文件头并缓存描述。 */
static uint8_t sd_assets_scan_image(uint8_t image_id)
{
    uint8_t header[SD_IMAGE_HEADER_SIZE];
    uint8_t slot;
    UINT got;

    slot = sd_image_slot(image_id);
    if (slot == 0xFFu) {
        return 0;
    }

    g_image_ready[slot] = 0;
    if (g_image_path[slot][0] == 0) {
        return 0;
    }
    if (!sd_assets_open_file(g_image_path[slot], SD_ASSET_TYPE_IMAGE, image_id)) {
        return 0;
    }
    if (f_lseek(&g_asset_file, 0) != FR_OK) {
        g_last_error = SD_ASSET_ERR_READ;
        return 0;
    }
    got = 0;
    if (f_read(&g_asset_file, header, SD_IMAGE_HEADER_SIZE, &got) != FR_OK ||
        got != SD_IMAGE_HEADER_SIZE) {
        g_last_error = SD_ASSET_ERR_HEADER;
        return 0;
    }
    if (!sd_assets_parse_image_header(header, &g_image_info[slot])) {
        if (g_last_error == SD_ASSET_ERR_NONE) {
            g_last_error = SD_ASSET_ERR_HEADER;
        }
        return 0;
    }

    g_image_ready[slot] = 1;
    return 1;
}

/* 读取 ASSET.IDX，按索引覆盖默认路径。 */
static uint8_t sd_assets_load_index(void)
{
    uint8_t header[SD_ASSET_INDEX_HEADER_SIZE];
    uint8_t entry[SD_ASSET_INDEX_ENTRY_SIZE];
    uint16_t count;
    uint16_t i;
    uint8_t slot;
    UINT got;

    if (!sd_assets_open_file(SD_ASSET_INDEX_NAME, 0xFEu, 0xFEu)) {
        return 0;
    }

    got = 0;
    if (f_read(&g_asset_file, header, SD_ASSET_INDEX_HEADER_SIZE, &got) != FR_OK ||
        got != SD_ASSET_INDEX_HEADER_SIZE) {
        g_last_error = SD_ASSET_ERR_HEADER;
        return 0;
    }
    if (header[0] != SD_ASSET_INDEX_MAGIC_0 || header[1] != SD_ASSET_INDEX_MAGIC_1 ||
        header[2] != SD_ASSET_INDEX_MAGIC_2 || header[3] != SD_ASSET_INDEX_MAGIC_3 ||
        sd_u16(&header[4]) != SD_ASSET_INDEX_VERSION) {
        g_last_error = SD_ASSET_ERR_HEADER;
        return 0;
    }

    count = sd_u16(&header[6]);
    for (i = 0; i < count; i++) {
        got = 0;
        if (f_read(&g_asset_file, entry, SD_ASSET_INDEX_ENTRY_SIZE, &got) != FR_OK ||
            got != SD_ASSET_INDEX_ENTRY_SIZE) {
            g_last_error = SD_ASSET_ERR_READ;
            return 0;
        }
        if (entry[0] == SD_ASSET_TYPE_IMAGE) {
            slot = sd_image_slot(entry[1]);
            if (slot != 0xFFu) {
                sd_copy_index_path(g_image_path[slot], &entry[4]);
            }
        } else if (entry[0] == SD_ASSET_TYPE_TEXT) {
            if (entry[1] == SD_ASSET_TEXT_FONT24) {
                sd_copy_index_path(g_font24_path, &entry[4]);
            } else if (entry[1] == SD_ASSET_TEXT_BOOK) {
                sd_copy_index_path(g_book_path, &entry[4]);
            } else if (entry[1] == SD_ASSET_TEXT_FONT16) {
                sd_copy_index_path(g_font16_path, &entry[4]);
            }
        }
    }

    g_last_error = SD_ASSET_ERR_NONE;
    return 1;
}

uint8_t sd_assets_reload(void)
{
    uint8_t i;

    sd_assets_close_file();
    sd_assets_reset_paths();
    for (i = 0; i < SD_ASSET_IMAGE_SLOT_COUNT; i++) {
        g_image_ready[i] = 0;
    }
    for (i = 0; i < SD_ASSET_FONT_SLOT_COUNT; i++) {
        g_font_ready[i] = 0;
    }
    g_assets_loaded = 0;

    if (!sd_assets_mount()) {
        return 0;
    }

    (void)sd_assets_load_index();
    for (i = SD_ASSET_IMAGE_MASCOT; i <= SD_ASSET_IMAGE_GIF4; i++) {
        (void)sd_assets_scan_image(i);
    }
    sd_assets_close_file();

    g_assets_loaded = 1;
    g_last_error = SD_ASSET_ERR_NONE;
    return 1;
}

/* 确保资源路径和图片头已经加载。 */
static uint8_t sd_assets_ensure_loaded(void)
{
    if (g_assets_loaded) {
        return 1;
    }
    return sd_assets_reload();
}

uint8_t sd_assets_get_image_info(uint8_t image_id, SdImageInfo *info)
{
    uint8_t slot;

    if (info == 0 || !sd_assets_ensure_loaded()) {
        return 0;
    }

    slot = sd_image_slot(image_id);
    if (slot == 0xFFu || !g_image_ready[slot]) {
        return 0;
    }
    *info = g_image_info[slot];
    return 1;
}

uint8_t sd_assets_begin_image_frame(uint8_t image_id, uint8_t index)
{
    uint8_t slot;
    DWORD offset;

    if (!sd_assets_ensure_loaded()) {
        return 0;
    }

    slot = sd_image_slot(image_id);
    if (slot == 0xFFu || !g_image_ready[slot]) {
        return 0;
    }
    if (index >= g_image_info[slot].frame_count) {
        index = 0;
    }
    if (!sd_assets_open_file(g_image_path[slot], SD_ASSET_TYPE_IMAGE, image_id)) {
        return 0;
    }

    offset = (DWORD)SD_IMAGE_HEADER_SIZE +
             (DWORD)index * (DWORD)g_image_info[slot].frame_bytes;
    if (f_lseek(&g_asset_file, offset) != FR_OK) {
        g_last_error = SD_ASSET_ERR_READ;
        return 0;
    }

    g_last_error = SD_ASSET_ERR_NONE;
    return 1;
}

uint8_t sd_assets_read_image_row(uint8_t *row, uint16_t row_bytes)
{
    UINT got;

    if (!g_asset_open || row == 0 || row_bytes > SD_ASSET_ROW_MAX_BYTES) {
        return 0;
    }

    got = 0;
    if (f_read(&g_asset_file, row, row_bytes, &got) != FR_OK ||
        got != row_bytes) {
        g_last_error = SD_ASSET_ERR_READ;
        return 0;
    }

    g_last_error = SD_ASSET_ERR_NONE;
    return 1;
}

/* 读取并缓存某个 SFNT 字库文件头。 */
static uint8_t sd_assets_load_font(uint8_t font_id)
{
    uint8_t header[SD_FONT_HEADER_READ_SIZE];
    uint8_t slot;
    uint16_t version;
    uint16_t header_size;
    uint16_t entry_size;
    UINT got;
    char *path;

    slot = sd_font_slot(font_id);
    path = sd_font_path(font_id);
    if (slot == 0xFFu || path == 0 || !sd_assets_ensure_loaded()) {
        return 0;
    }
    if (g_font_ready[slot]) {
        return 1;
    }
    if (!sd_assets_open_file(path, SD_ASSET_TYPE_TEXT, font_id)) {
        return 0;
    }
    if (f_lseek(&g_asset_file, 0) != FR_OK) {
        g_last_error = SD_ASSET_ERR_READ;
        return 0;
    }

    got = 0;
    if (f_read(&g_asset_file, header, SD_FONT_HEADER_READ_SIZE, &got) != FR_OK ||
        got != SD_FONT_HEADER_READ_SIZE) {
        g_last_error = SD_ASSET_ERR_HEADER;
        return 0;
    }
    version = sd_u16(&header[4]);
    header_size = sd_u16(&header[6]);
    if (header[0] != SD_FONT_MAGIC_0 || header[1] != SD_FONT_MAGIC_1 ||
        header[2] != SD_FONT_MAGIC_2 || header[3] != SD_FONT_MAGIC_3 ||
        (version != SD_FONT_VERSION_V1 &&
         version != SD_FONT_VERSION_V2 &&
         version != SD_FONT_VERSION_V3)) {
        g_last_error = SD_ASSET_ERR_HEADER;
        return 0;
    }
    if ((version == SD_FONT_VERSION_V3 && header_size != SD_FONT_HEADER_SIZE_V3) ||
        (version != SD_FONT_VERSION_V3 && header_size != SD_FONT_HEADER_SIZE)) {
        g_last_error = SD_ASSET_ERR_HEADER;
        return 0;
    }

    g_font_width[slot] = sd_u16(&header[8]);
    g_font_height[slot] = sd_u16(&header[10]);
    g_font_stride[slot] = sd_u16(&header[12]);
    g_font_count[slot] = sd_u16(&header[14]);
    g_font_version[slot] = (uint8_t)version;
    if (version == SD_FONT_VERSION_V3) {
        entry_size = sd_u16(&header[16]);
        if (entry_size != SD_FONT_ENTRY_SIZE_V2) {
            g_last_error = SD_ASSET_ERR_HEADER;
            return 0;
        }
        g_font_entry_size[slot] = entry_size;
        g_font_lookup_offset[slot] = sd_u32(&header[18]);
    } else {
        g_font_entry_size[slot] = (version == SD_FONT_VERSION_V2) ?
                                  SD_FONT_ENTRY_SIZE_V2 : SD_FONT_ENTRY_SIZE_V1;
        g_font_lookup_offset[slot] = 0;
    }
    g_font_header_size[slot] = header_size;

    if (g_font_width[slot] == 0 || g_font_height[slot] == 0 ||
        g_font_stride[slot] == 0 ||
        g_font_width[slot] > (uint16_t)(g_font_stride[slot] * 8u) ||
        g_font_stride[slot] > SD_ASSET_ROW_MAX_BYTES) {
        g_last_error = SD_ASSET_ERR_SIZE;
        return 0;
    }

    g_font_ready[slot] = 1;
    g_last_error = SD_ASSET_ERR_NONE;
    return 1;
}

/* 在指定 SFNT 字库中二分查找码点，并把文件指针定位到字形数据。 */
static uint8_t sd_assets_begin_glyph_from_font(uint8_t font_id, uint16_t codepoint, SdGlyphInfo *info)
{
    uint8_t entry[SD_FONT_ENTRY_SIZE_V2];
    uint8_t slot;
    uint16_t low;
    uint16_t high;
    uint16_t mid;
    uint16_t entry_codepoint;
    uint16_t bytes;
    uint32_t data_offset;
    uint32_t entry_offset;
    uint16_t glyph_bytes;
    UINT got;
    char *path;

    if (info == 0 || !sd_assets_load_font(font_id)) {
        return 0;
    }

    slot = sd_font_slot(font_id);
    path = sd_font_path(font_id);
    glyph_bytes = (uint16_t)(g_font_stride[slot] * g_font_height[slot]);
    low = 0;
    high = g_font_count[slot];

    if (!sd_assets_open_file(path, SD_ASSET_TYPE_TEXT, font_id)) {
        return 0;
    }

    if (g_font_version[slot] == SD_FONT_VERSION_V3) {
        data_offset = g_font_lookup_offset[slot] + (uint32_t)codepoint * 4u;
        if (f_lseek(&g_asset_file, (DWORD)data_offset) != FR_OK) {
            g_last_error = SD_ASSET_ERR_READ;
            return 0;
        }
        got = 0;
        if (f_read(&g_asset_file, entry, 4, &got) != FR_OK || got != 4) {
            g_last_error = SD_ASSET_ERR_READ;
            return 0;
        }
        entry_offset = sd_u32(entry);
        if (entry_offset == SD_FONT_DIRECT_MISSING) {
            g_last_error = SD_ASSET_ERR_OPEN;
            return 0;
        }
        if (f_lseek(&g_asset_file, (DWORD)entry_offset) != FR_OK) {
            g_last_error = SD_ASSET_ERR_READ;
            return 0;
        }
        got = 0;
        if (f_read(&g_asset_file, entry, g_font_entry_size[slot], &got) != FR_OK ||
            got != g_font_entry_size[slot]) {
            g_last_error = SD_ASSET_ERR_READ;
            return 0;
        }
        if (sd_u16(&entry[0]) != codepoint) {
            g_last_error = SD_ASSET_ERR_HEADER;
            return 0;
        }
        data_offset = sd_u32(&entry[4]);
        bytes = sd_u16(&entry[8]);
        if (bytes != glyph_bytes) {
            g_last_error = SD_ASSET_ERR_SIZE;
            return 0;
        }
        if (f_lseek(&g_asset_file, (DWORD)data_offset) != FR_OK) {
            g_last_error = SD_ASSET_ERR_READ;
            return 0;
        }

        info->width = g_font_width[slot];
        info->height = g_font_height[slot];
        info->stride = g_font_stride[slot];
        g_last_error = SD_ASSET_ERR_NONE;
        return 1;
    }

    while (low < high) {
        mid = (uint16_t)(low + ((uint16_t)(high - low) >> 1));
        data_offset = (uint32_t)g_font_header_size[slot] +
                      (uint32_t)mid * (uint32_t)g_font_entry_size[slot];
        if (f_lseek(&g_asset_file, (DWORD)data_offset) != FR_OK) {
            g_last_error = SD_ASSET_ERR_READ;
            return 0;
        }
        got = 0;
        if (f_read(&g_asset_file, entry, g_font_entry_size[slot], &got) != FR_OK ||
            got != g_font_entry_size[slot]) {
            g_last_error = SD_ASSET_ERR_READ;
            return 0;
        }

        entry_codepoint = sd_u16(&entry[0]);
        if (entry_codepoint < codepoint) {
            low = (uint16_t)(mid + 1u);
            continue;
        }
        if (entry_codepoint > codepoint) {
            high = mid;
            continue;
        }

        if (g_font_version[slot] == SD_FONT_VERSION_V2) {
            data_offset = sd_u32(&entry[4]);
            bytes = sd_u16(&entry[8]);
        } else {
            data_offset = sd_u16(&entry[2]);
            bytes = sd_u16(&entry[4]);
        }
        if (bytes != glyph_bytes) {
            g_last_error = SD_ASSET_ERR_SIZE;
            return 0;
        }
        if (f_lseek(&g_asset_file, (DWORD)data_offset) != FR_OK) {
            g_last_error = SD_ASSET_ERR_READ;
            return 0;
        }

        info->width = g_font_width[slot];
        info->height = g_font_height[slot];
        info->stride = g_font_stride[slot];
        g_last_error = SD_ASSET_ERR_NONE;
        return 1;
    }

    g_last_error = SD_ASSET_ERR_OPEN;
    return 0;
}

uint8_t sd_assets_begin_glyph(uint16_t codepoint, SdGlyphInfo *info)
{
    return sd_assets_begin_glyph_from_font(SD_ASSET_TEXT_FONT24, codepoint, info);
}

uint8_t sd_assets_begin_text_glyph(uint16_t codepoint, SdGlyphInfo *info)
{
    return sd_assets_begin_glyph_from_font(SD_ASSET_TEXT_FONT16, codepoint, info);
}

uint8_t sd_assets_read_glyph_row(uint8_t *row, uint16_t row_bytes)
{
    return sd_assets_read_image_row(row, row_bytes);
}

uint8_t sd_assets_begin_text(uint32_t offset, uint32_t *size)
{
    if (!sd_assets_ensure_loaded()) {
        return 0;
    }
    if (!sd_assets_open_file(g_book_path, SD_ASSET_TYPE_TEXT, SD_ASSET_TEXT_BOOK)) {
        return 0;
    }
    if (size != 0) {
        *size = (uint32_t)f_size(&g_asset_file);
    }
    if (offset > (uint32_t)f_size(&g_asset_file)) {
        offset = (uint32_t)f_size(&g_asset_file);
    }
    if (f_lseek(&g_asset_file, (DWORD)offset) != FR_OK) {
        g_last_error = SD_ASSET_ERR_READ;
        return 0;
    }
    g_last_error = SD_ASSET_ERR_NONE;
    return 1;
}

uint8_t sd_assets_read_text_byte(uint8_t *value)
{
    UINT got;

    if (value == 0 ||
        !g_asset_open ||
        g_asset_open_type != SD_ASSET_TYPE_TEXT ||
        g_asset_open_id != SD_ASSET_TEXT_BOOK) {
        return 0;
    }
    got = 0;
    if (f_read(&g_asset_file, value, 1, &got) != FR_OK) {
        g_last_error = SD_ASSET_ERR_READ;
        return 0;
    }
    if (got != 1) {
        return 0;
    }
    g_last_error = SD_ASSET_ERR_NONE;
    return 1;
}

uint32_t sd_assets_text_tell(void)
{
    if (!g_asset_open ||
        g_asset_open_type != SD_ASSET_TYPE_TEXT ||
        g_asset_open_id != SD_ASSET_TEXT_BOOK) {
        return 0;
    }
    return (uint32_t)f_tell(&g_asset_file);
}

uint8_t sd_assets_first_gif_image(void)
{
    uint8_t i;
    uint8_t slot;

    if (!sd_assets_ensure_loaded()) {
        return 0;
    }
    for (i = 0; i < (uint8_t)(sizeof(g_gif_image_ids) / sizeof(g_gif_image_ids[0])); i++) {
        slot = sd_image_slot(g_gif_image_ids[i]);
        if (slot != 0xFFu && g_image_ready[slot]) {
            return g_gif_image_ids[i];
        }
    }
    return 0;
}

uint8_t sd_assets_step_gif_image(uint8_t current_id, int8_t direction)
{
    uint8_t i;
    uint8_t start;
    uint8_t pos;
    uint8_t slot;
    uint8_t count;

    if (!sd_assets_ensure_loaded()) {
        return 0;
    }

    count = (uint8_t)(sizeof(g_gif_image_ids) / sizeof(g_gif_image_ids[0]));
    start = 0;
    for (i = 0; i < count; i++) {
        if (g_gif_image_ids[i] == current_id) {
            start = i;
            break;
        }
    }

    pos = start;
    for (i = 0; i < count; i++) {
        if (direction < 0) {
            pos = (pos == 0) ? (uint8_t)(count - 1u) : (uint8_t)(pos - 1u);
        } else {
            pos++;
            if (pos >= count) {
                pos = 0;
            }
        }
        slot = sd_image_slot(g_gif_image_ids[pos]);
        if (slot != 0xFFu && g_image_ready[slot]) {
            return g_gif_image_ids[pos];
        }
    }
    return sd_assets_first_gif_image();
}

uint8_t sd_assets_write_probe(void)
{
    static const char line[] = "MSP430 FAT32 write ok\r\n";
    FIL file;
    UINT wrote;

    if (!sd_assets_mount()) {
        return 0;
    }
    if (f_open(&file, SD_ASSET_PROBE_NAME, FA_WRITE | FA_OPEN_ALWAYS) != FR_OK) {
        g_last_error = SD_ASSET_ERR_WRITE;
        return 0;
    }
    (void)f_lseek(&file, file.fsize);
    wrote = 0;
    if (f_write(&file, line, (UINT)(sizeof(line) - 1u), &wrote) != FR_OK ||
        wrote != (UINT)(sizeof(line) - 1u)) {
        (void)f_close(&file);
        g_last_error = SD_ASSET_ERR_WRITE;
        return 0;
    }
    if (f_close(&file) != FR_OK) {
        g_last_error = SD_ASSET_ERR_WRITE;
        return 0;
    }
    g_last_error = SD_ASSET_ERR_NONE;
    return 1;
}

uint8_t sd_assets_last_error(void)
{
    return g_last_error;
}
