/*
 * sd_assets.c
 * FAT32 SD 卡资源读取层：通过 ASSET.IDX 找到 IMG 和 TEXT 目录中的资源，
 * 图片和字形都按行读取，避免在 RAM 中保存整张图或整套字库。
 */
#include "sd_assets.h"

#include "fatfs/ff.h"

#define SD_ASSET_INDEX_NAME         "ASSET.IDX"      /* 根目录资源索引文件名。 */
#define SD_ASSET_PROBE_NAME         "SDTEST.TXT"     /* FAT32 写入探针文件名。 */
#define SD_ASSET_DEFAULT_MASCOT     "IMG/MASCOT.BIN" /* S1 全屏 GIF 默认路径。 */
#define SD_ASSET_DEFAULT_HOURGLASS  "IMG/HOURGLAS.BIN" /* 主界面沙漏默认路径，文件名保持 8.3。 */
#define SD_ASSET_DEFAULT_FONT24     "TEXT/FONT24.BIN" /* 24x24 点阵字库默认路径。 */

#define SD_ASSET_PATH_LEN           20u              /* 索引中保存的 8.3 路径最大长度，含结束符。 */
#define SD_ASSET_INDEX_HEADER_SIZE  8u               /* ASSET.IDX 文件头长度。 */
#define SD_ASSET_INDEX_ENTRY_SIZE   24u              /* ASSET.IDX 单条索引长度。 */
#define SD_ASSET_INDEX_VERSION      1u               /* ASSET.IDX 格式版本。 */
#define SD_ASSET_INDEX_MAGIC_0      'A'              /* ASSET.IDX 魔数第 0 字节。 */
#define SD_ASSET_INDEX_MAGIC_1      'I'              /* ASSET.IDX 魔数第 1 字节。 */
#define SD_ASSET_INDEX_MAGIC_2      'D'              /* ASSET.IDX 魔数第 2 字节。 */
#define SD_ASSET_INDEX_MAGIC_3      'X'              /* ASSET.IDX 魔数第 3 字节。 */
#define SD_ASSET_TYPE_IMAGE         1u               /* ASSET.IDX 类型：图片资源。 */
#define SD_ASSET_TYPE_TEXT          2u               /* ASSET.IDX 类型：文字资源。 */
#define SD_ASSET_TEXT_FONT24        1u               /* TEXT 目录资源 ID：24x24 点阵字库。 */

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
#define SD_FONT_VERSION             1u               /* SFNT 字库格式版本。 */
#define SD_FONT_HEADER_SIZE         16u              /* SFNT 字库文件头长度。 */
#define SD_FONT_ENTRY_SIZE          8u               /* SFNT 字库单个字形索引长度。 */

#define SD_ASSET_ERR_NONE           0u               /* 最近一次操作成功。 */
#define SD_ASSET_ERR_MOUNT          1u               /* FAT32 挂载失败。 */
#define SD_ASSET_ERR_OPEN           2u               /* 打开资源文件失败。 */
#define SD_ASSET_ERR_HEADER         3u               /* 资源文件头读取或校验失败。 */
#define SD_ASSET_ERR_SIZE           4u               /* 资源行宽超过内部缓存。 */
#define SD_ASSET_ERR_READ           5u               /* 读取资源数据失败。 */
#define SD_ASSET_ERR_WRITE          6u               /* 写入探针文件失败。 */

static FATFS g_sd_fs;
static FIL g_asset_file;
static SdImageInfo g_image_info[2];
static uint8_t g_sd_mounted = 0;
static uint8_t g_assets_loaded = 0;
static uint8_t g_asset_open = 0;
static uint8_t g_asset_open_type = 0;
static uint8_t g_asset_open_id = 0;
static uint8_t g_image_ready[2] = {0, 0};
static uint8_t g_last_error = SD_ASSET_ERR_NONE;
static char g_image_path[2][SD_ASSET_PATH_LEN] = {
    SD_ASSET_DEFAULT_MASCOT,
    SD_ASSET_DEFAULT_HOURGLASS
};
static char g_font24_path[SD_ASSET_PATH_LEN] = SD_ASSET_DEFAULT_FONT24;

/* 从小端字节流读取 16 位无符号数。 */
static uint16_t sd_u16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

/* 从小端字节流读取 16 位有符号数。 */
static int16_t sd_i16(const uint8_t *p)
{
    return (int16_t)sd_u16(p);
}

/* 按资源 ID 转换成图片槽位。 */
static uint8_t sd_image_slot(uint8_t image_id)
{
    if (image_id == SD_ASSET_IMAGE_MASCOT) {
        return 0;
    }
    if (image_id == SD_ASSET_IMAGE_HOURGLASS) {
        return 1;
    }
    return 0xFFu;
}

/* 把路径缓冲恢复成标准目录结构，索引缺失时仍可按约定路径读取。 */
static void sd_assets_reset_paths(void)
{
    static const char mascot[] = SD_ASSET_DEFAULT_MASCOT;
    static const char hourglass[] = SD_ASSET_DEFAULT_HOURGLASS;
    static const char font24[] = SD_ASSET_DEFAULT_FONT24;
    uint8_t i;

    for (i = 0; i < SD_ASSET_PATH_LEN; i++) {
        g_image_path[0][i] = (i < (sizeof(mascot) - 1u)) ? mascot[i] : 0;
        g_image_path[1][i] = (i < (sizeof(hourglass) - 1u)) ? hourglass[i] : 0;
        g_font24_path[i] = (i < (sizeof(font24) - 1u)) ? font24[i] : 0;
    }
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
            if (entry[1] == SD_ASSET_IMAGE_MASCOT) {
                sd_copy_index_path(g_image_path[0], &entry[4]);
            } else if (entry[1] == SD_ASSET_IMAGE_HOURGLASS) {
                sd_copy_index_path(g_image_path[1], &entry[4]);
            }
        } else if (entry[0] == SD_ASSET_TYPE_TEXT && entry[1] == SD_ASSET_TEXT_FONT24) {
            sd_copy_index_path(g_font24_path, &entry[4]);
        }
    }

    g_last_error = SD_ASSET_ERR_NONE;
    return 1;
}

uint8_t sd_assets_reload(void)
{
    sd_assets_close_file();
    sd_assets_reset_paths();
    g_image_ready[0] = 0;
    g_image_ready[1] = 0;
    g_assets_loaded = 0;

    if (!sd_assets_mount()) {
        return 0;
    }

    (void)sd_assets_load_index();
    (void)sd_assets_scan_image(SD_ASSET_IMAGE_MASCOT);
    (void)sd_assets_scan_image(SD_ASSET_IMAGE_HOURGLASS);
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

uint8_t sd_assets_begin_glyph(uint16_t codepoint, SdGlyphInfo *info)
{
    uint8_t header[SD_FONT_HEADER_SIZE];
    uint8_t entry[SD_FONT_ENTRY_SIZE];
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    uint16_t count;
    uint16_t i;
    uint16_t offset;
    uint16_t bytes;
    UINT got;

    if (info == 0 || !sd_assets_ensure_loaded()) {
        return 0;
    }
    if (!sd_assets_open_file(g_font24_path, SD_ASSET_TYPE_TEXT, SD_ASSET_TEXT_FONT24)) {
        return 0;
    }
    if (f_lseek(&g_asset_file, 0) != FR_OK) {
        g_last_error = SD_ASSET_ERR_READ;
        return 0;
    }

    got = 0;
    if (f_read(&g_asset_file, header, SD_FONT_HEADER_SIZE, &got) != FR_OK ||
        got != SD_FONT_HEADER_SIZE) {
        g_last_error = SD_ASSET_ERR_HEADER;
        return 0;
    }
    if (header[0] != SD_FONT_MAGIC_0 || header[1] != SD_FONT_MAGIC_1 ||
        header[2] != SD_FONT_MAGIC_2 || header[3] != SD_FONT_MAGIC_3 ||
        sd_u16(&header[4]) != SD_FONT_VERSION ||
        sd_u16(&header[6]) != SD_FONT_HEADER_SIZE) {
        g_last_error = SD_ASSET_ERR_HEADER;
        return 0;
    }

    width = sd_u16(&header[8]);
    height = sd_u16(&header[10]);
    stride = sd_u16(&header[12]);
    count = sd_u16(&header[14]);
    if (width == 0 || height == 0 || stride == 0 ||
        width > (uint16_t)(stride * 8u) || stride > SD_ASSET_ROW_MAX_BYTES) {
        g_last_error = SD_ASSET_ERR_SIZE;
        return 0;
    }

    for (i = 0; i < count; i++) {
        got = 0;
        if (f_read(&g_asset_file, entry, SD_FONT_ENTRY_SIZE, &got) != FR_OK ||
            got != SD_FONT_ENTRY_SIZE) {
            g_last_error = SD_ASSET_ERR_READ;
            return 0;
        }
        if (sd_u16(&entry[0]) != codepoint) {
            continue;
        }

        offset = sd_u16(&entry[2]);
        bytes = sd_u16(&entry[4]);
        if (bytes != (uint16_t)(stride * height)) {
            g_last_error = SD_ASSET_ERR_SIZE;
            return 0;
        }
        if (f_lseek(&g_asset_file, offset) != FR_OK) {
            g_last_error = SD_ASSET_ERR_READ;
            return 0;
        }

        info->width = width;
        info->height = height;
        info->stride = stride;
        g_last_error = SD_ASSET_ERR_NONE;
        return 1;
    }

    g_last_error = SD_ASSET_ERR_OPEN;
    return 0;
}

uint8_t sd_assets_read_glyph_row(uint8_t *row, uint16_t row_bytes)
{
    return sd_assets_read_image_row(row, row_bytes);
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
