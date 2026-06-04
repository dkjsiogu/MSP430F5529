#include "flash_log.h"

#include "app_state.h"
#include "format.h"

#define FLASH_RECORD_COUNT          ((FLASH_LOG_END - FLASH_LOG_START) / sizeof(TempRecord)) /* Flash 历史记录区最多能容纳的温度记录数。 */

static uint16_t g_flash_next_index = 0;
static uint16_t g_next_seq = 0;

/* 计算温度历史记录 CRC，用来发现 Flash 中未写完或损坏的记录。 */
static uint16_t record_crc(const TempRecord *r)
{
    const uint16_t *w;
    uint8_t i;
    uint16_t crc;

    w = (const uint16_t *)r;
    crc = 0x5A5Au;
    for (i = 0; i < (uint8_t)((sizeof(TempRecord) / 2u) - 1u); i++) {
        crc = (uint16_t)((crc << 1) | (crc >> 15));
        crc ^= w[i];
    }
    return crc;
}

/* 判断一条 Flash 温度记录的 magic 和 CRC 是否有效。 */
static uint8_t record_valid(const TempRecord *r)
{
    if (r->magic != RECORD_MAGIC) {
        return 0;
    }
    return (uint8_t)(record_crc(r) == r->crc);
}

/* 擦除从 addr 开始的一个主 Flash 段。 */
static void flash_erase_segment(uint16_t addr)
{
    while (FCTL3 & BUSY) {
        ;
    }
    FCTL3 = FWKEY;
    FCTL1 = FWKEY | ERASE;
    *(volatile uint16_t *)addr = 0;
    while (FCTL3 & BUSY) {
        ;
    }
    FCTL1 = FWKEY;
    FCTL3 = FWKEY | LOCK;
}

void flash_erase_log(void)
{
    uint16_t addr;

    __disable_interrupt();
    for (addr = FLASH_LOG_START; addr < FLASH_LOG_END; addr = (uint16_t)(addr + FLASH_SEGMENT_SIZE)) {
        flash_erase_segment(addr);
    }
    g_flash_next_index = 0;
    g_next_seq = 0;
    __enable_interrupt();
}

/* 把一条温度记录写入 Flash 历史记录区的指定下标。 */
static void flash_write_record(uint16_t index, const TempRecord *r)
{
    const uint16_t *src;
    volatile uint16_t *dst;
    uint8_t words;
    uint8_t i;

    src = (const uint16_t *)r;
    dst = (volatile uint16_t *)(FLASH_LOG_START + index * sizeof(TempRecord));
    words = (uint8_t)(sizeof(TempRecord) / 2u);

    __disable_interrupt();
    while (FCTL3 & BUSY) {
        ;
    }
    FCTL3 = FWKEY;
    FCTL1 = FWKEY | WRT;
    for (i = 0; i < words; i++) {
        dst[i] = src[i];
    }
    while (FCTL3 & BUSY) {
        ;
    }
    FCTL1 = FWKEY;
    FCTL3 = FWKEY | LOCK;
    __enable_interrupt();
}

void flash_scan(void)
{
    uint16_t i;
    const TempRecord *r;

    g_flash_next_index = 0;
    g_next_seq = 0;

    for (i = 0; i < FLASH_RECORD_COUNT; i++) {
        r = (const TempRecord *)(FLASH_LOG_START + i * sizeof(TempRecord));
        if (r->magic == 0xFFFFu) {
            g_flash_next_index = i;
            return;
        }
        if (!record_valid(r)) {
            g_flash_next_index = i;
            return;
        }
        g_flash_next_index = (uint16_t)(i + 1u);
        g_next_seq = (uint16_t)(r->seq + 1u);
    }
}

void flash_log_sample(const TempSample *s)
{
    TempRecord r;

    if (g_flash_next_index >= FLASH_RECORD_COUNT || g_flash_next_index >= app_storage_limit()) {
        flash_erase_log();
    }

    r.magic = RECORD_MAGIC;
    r.seq = g_next_seq++;
    r.die_t10 = s->die_t10;
    r.ntc_t10 = s->ntc_t10;
    r.tmp_local_t10 = s->tmp_local_t10;
    r.reserved_t10 = s->reserved_t10;
    r.flags = s->flags;
    r.crc = record_crc(&r);

    flash_write_record(g_flash_next_index, &r);
    g_flash_next_index++;
}

uint16_t history_count(void)
{
    uint16_t i;
    uint16_t limit;
    const TempRecord *r;

    limit = app_storage_limit();
    if (limit > FLASH_RECORD_COUNT) {
        limit = FLASH_RECORD_COUNT;
    }

    for (i = 0; i < limit; i++) {
        r = (const TempRecord *)(FLASH_LOG_START + i * sizeof(TempRecord));
        if (!record_valid(r)) {
            return i;
        }
    }
    return limit;
}

uint8_t history_get(uint16_t index, TempRecord *out)
{
    const TempRecord *r;

    if (index >= FLASH_RECORD_COUNT || index >= app_storage_limit()) {
        return 0;
    }
    r = (const TempRecord *)(FLASH_LOG_START + index * sizeof(TempRecord));
    if (!record_valid(r)) {
        return 0;
    }
    *out = *r;
    return 1;
}

