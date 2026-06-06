/*
 * text_reader.c
 * 文本阅读器实现：从资源层流式读取 UTF-8 BOOK.TXT，
 * 只缓存当前页的字符坐标，适合 MSP430 的 RAM 约束。
 */
#include "text_reader.h"

#include "app_resources.h"

#define TEXT_READER_MARGIN_X        2u                   /* 阅读页左侧留白，单位像素。 */

#pragma DATA_SECTION(g_text_items, ".bss:usbram")
static TextReaderItem g_text_items[TEXT_READER_MAX_ITEMS];
static uint8_t g_text_item_count = 0;
static uint32_t g_text_page_offset = 0;
static uint32_t g_text_next_offset = 0;
static uint32_t g_text_file_size = 0;
#pragma DATA_SECTION(g_text_prev_offsets, ".bss:usbram")
static uint32_t g_text_prev_offsets[TEXT_READER_HISTORY_DEPTH];
static uint8_t g_text_prev_count = 0;

/* 从 BOOK.TXT 当前文件位置读取一个字节，并同步维护解析偏移。 */
static uint8_t text_reader_read_byte(uint32_t *offset, uint8_t *value)
{
    if (!app_resources_read_book_byte(value)) {
        return 0;
    }
    *offset = (uint32_t)(*offset + 1u);
    return 1;
}

/* 从 UTF-8 文本流中解码一个 BMP 码点，返回值 0 表示到达文件尾。 */
static uint8_t text_reader_decode_next(uint32_t *offset, uint16_t *codepoint, uint8_t *skip_lf)
{
    uint8_t b0;
    uint8_t b1;
    uint8_t b2;
    uint32_t start;

    while (text_reader_read_byte(offset, &b0)) {
        start = (uint32_t)(*offset - 1u);
        if (start == 0 && b0 == 0xEFu) {
            if (text_reader_read_byte(offset, &b1) &&
                text_reader_read_byte(offset, &b2) &&
                b1 == 0xBBu && b2 == 0xBFu) {
                continue;
            }
            *codepoint = '?';
            return 1;
        }

        if (*skip_lf && b0 == '\n') {
            *skip_lf = 0;
            continue;
        }
        *skip_lf = 0;

        if (b0 == '\r') {
            *skip_lf = 1;
            *codepoint = '\n';
            return 1;
        }
        if (b0 == '\n') {
            *codepoint = '\n';
            return 1;
        }
        if (b0 < 0x80u) {
            *codepoint = b0;
            return 1;
        }
        if ((b0 & 0xE0u) == 0xC0u) {
            if (!text_reader_read_byte(offset, &b1)) {
                *codepoint = '?';
                return 1;
            }
            *codepoint = (uint16_t)(((uint16_t)(b0 & 0x1Fu) << 6) |
                                    (uint16_t)(b1 & 0x3Fu));
            return 1;
        }
        if ((b0 & 0xF0u) == 0xE0u) {
            if (!text_reader_read_byte(offset, &b1) ||
                !text_reader_read_byte(offset, &b2)) {
                *codepoint = '?';
                return 1;
            }
            *codepoint = (uint16_t)(((uint16_t)(b0 & 0x0Fu) << 12) |
                                    ((uint16_t)(b1 & 0x3Fu) << 6) |
                                    (uint16_t)(b2 & 0x3Fu));
            return 1;
        }

        *codepoint = '?';
        return 1;
    }
    return 0;
}

/* 判断阅读页字符是否只占空白位置。 */
static uint8_t text_reader_is_space(uint16_t codepoint)
{
    return (uint8_t)(codepoint == ' ' ||
                     codepoint == '\t' ||
                     codepoint == 0x3000u);
}

/* 返回阅读页中一个码点的排版宽度。 */
static uint8_t text_reader_char_width(uint16_t codepoint)
{
    if (codepoint < 128u) {
        return TEXT_READER_ASCII_W;
    }
    return TEXT_READER_CELL_W;
}

void text_reader_reset(void)
{
    g_text_item_count = 0;
    g_text_page_offset = 0;
    g_text_next_offset = 0;
    g_text_file_size = 0;
    g_text_prev_count = 0;
}

uint8_t text_reader_build_page(uint16_t screen_w, uint8_t top_y, uint8_t line_count, TextReaderPage *page)
{
    uint32_t offset;
    uint32_t item_offset;
    uint16_t codepoint;
    uint16_t x;
    uint16_t y;
    uint8_t line;
    uint8_t width;
    uint8_t skip_lf;

    if (page == 0) {
        return 0;
    }
    page->items = g_text_items;
    page->count = 0;
    page->end = 0;
    page->file_size = 0;
    g_text_item_count = 0;

    if (!app_resources_begin_book(g_text_page_offset, &g_text_file_size)) {
        g_text_next_offset = g_text_page_offset;
        return 0;
    }
    if (g_text_page_offset > g_text_file_size) {
        g_text_page_offset = 0;
        if (!app_resources_begin_book(0, &g_text_file_size)) {
            g_text_next_offset = 0;
            return 0;
        }
    }

    offset = g_text_page_offset;
    x = TEXT_READER_MARGIN_X;
    y = top_y;
    line = 0;
    skip_lf = 0;

    while (line < line_count && g_text_item_count < TEXT_READER_MAX_ITEMS) {
        item_offset = offset;
        if (!text_reader_decode_next(&offset, &codepoint, &skip_lf)) {
            break;
        }

        if (codepoint == '\n') {
            line++;
            x = TEXT_READER_MARGIN_X;
            y = (uint16_t)(y + TEXT_READER_LINE_HEIGHT);
            continue;
        }
        if (codepoint == '\t') {
            codepoint = ' ';
        }

        width = text_reader_char_width(codepoint);
        if ((uint16_t)(x + width) > screen_w) {
            line++;
            if (line >= line_count) {
                offset = item_offset;
                break;
            }
            x = TEXT_READER_MARGIN_X;
            y = (uint16_t)(y + TEXT_READER_LINE_HEIGHT);
        }
        if (text_reader_is_space(codepoint) && x == TEXT_READER_MARGIN_X) {
            continue;
        }
        if (!text_reader_is_space(codepoint)) {
            g_text_items[g_text_item_count].codepoint = codepoint;
            g_text_items[g_text_item_count].x = (uint8_t)x;
            g_text_items[g_text_item_count].y = (uint8_t)y;
            g_text_item_count++;
        }
        x = (uint16_t)(x + width);
    }

    if (offset >= g_text_file_size) {
        g_text_next_offset = 0;
        page->end = (uint8_t)(g_text_file_size != 0);
    } else {
        g_text_next_offset = offset;
    }
    page->count = g_text_item_count;
    page->file_size = g_text_file_size;
    return 1;
}

void text_reader_prev_page(void)
{
    if (g_text_prev_count > 0) {
        g_text_prev_count--;
        g_text_page_offset = g_text_prev_offsets[g_text_prev_count];
    } else {
        g_text_page_offset = 0;
    }
}

void text_reader_next_page(void)
{
    uint8_t i;

    if (g_text_prev_count >= TEXT_READER_HISTORY_DEPTH) {
        for (i = 1; i < TEXT_READER_HISTORY_DEPTH; i++) {
            g_text_prev_offsets[i - 1u] = g_text_prev_offsets[i];
        }
        g_text_prev_count = (uint8_t)(TEXT_READER_HISTORY_DEPTH - 1u);
    }
    g_text_prev_offsets[g_text_prev_count] = g_text_page_offset;
    g_text_prev_count++;
    g_text_page_offset = g_text_next_offset;
}
