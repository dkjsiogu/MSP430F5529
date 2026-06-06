/*
 * text_reader.h
 * 文本阅读器：负责 BOOK.TXT 的 UTF-8 解码、分页、上一页栈和阅读偏移，
 * 渲染层只按返回的字符列表绘制点阵。
 */
#ifndef TEXT_READER_H
#define TEXT_READER_H                                    /* 防止 text_reader.h 被重复包含。 */

#include <stdint.h>

#define TEXT_READER_MAX_ITEMS       128u                 /* 单页最多缓存的字符数量。 */
#define TEXT_READER_HISTORY_DEPTH   16u                  /* 向前翻页保存的页起点数量。 */
#define TEXT_READER_CELL_W          16u                  /* 中文点阵默认占位宽度。 */
#define TEXT_READER_ASCII_W         6u                   /* ASCII 字符默认占位宽度。 */
#define TEXT_READER_LINE_HEIGHT     18u                  /* 正文行高，匹配 16x16 点阵。 */

typedef struct {
    uint16_t codepoint;                                  /* UTF-8 解码后的 Unicode BMP 码点。 */
    uint8_t x;                                           /* 字符左上角 X 坐标。 */
    uint8_t y;                                           /* 字符左上角 Y 坐标。 */
} TextReaderItem;

typedef struct {
    const TextReaderItem *items;                         /* 当前页字符列表，只在下一次构页前有效。 */
    uint8_t count;                                       /* 当前页字符数量。 */
    uint8_t end;                                         /* 当前页是否已经到达文件末尾。 */
    uint32_t file_size;                                  /* BOOK.TXT 文件总字节数。 */
} TextReaderPage;

void text_reader_reset(void);                            /* 回到 BOOK.TXT 开头并清空上一页栈。 */
uint8_t text_reader_build_page(uint16_t screen_w, uint8_t top_y, uint8_t line_count, TextReaderPage *page); /* 解析当前页。 */
void text_reader_prev_page(void);                        /* 切换到上一页；没有上一页时保持开头。 */
void text_reader_next_page(void);                        /* 切换到下一页；到末尾后回到开头。 */

#endif
