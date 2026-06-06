#include "settings_store.h"

#include "app_config.h"
#include "platform_config.h"

typedef struct {
    uint16_t magic;
    uint16_t seq;
    uint8_t sample_interval;
    uint8_t alarm_duration;
    uint8_t hourglass_seconds;
    uint8_t reserved;
    int16_t threshold_t10;
    uint16_t storage_limit;
    uint16_t crc;
} SettingsRecord;

#define SETTINGS_RECORD_COUNT       ((SETTINGS_FLASH_END - SETTINGS_FLASH_START) / sizeof(SettingsRecord)) /* Info Flash 设置区最多可保存的记录数。 */
#define SETTINGS_FLASH_WAIT_GUARD   1000000UL

static uint16_t g_settings_next_index = 0;
static uint16_t g_settings_next_seq = 0;

/* 等待 MSP430 Flash 控制器空闲，超时返回 0，避免异常时永久死等。 */
static uint8_t settings_flash_wait_ready(void)
{
    uint32_t guard;

    guard = SETTINGS_FLASH_WAIT_GUARD;
    while ((FCTL3 & BUSY) && guard > 0u) {
        guard--;
    }
    return (uint8_t)(guard > 0u);
}

/* 计算设置记录 CRC16，用于发现 Info Flash 中未写完或损坏的记录。 */
static uint16_t settings_crc16_update(uint16_t crc, uint8_t data)
{
    uint8_t bit;

    crc ^= (uint16_t)data << 8;
    for (bit = 0; bit < 8u; bit++) {
        if (crc & 0x8000u) {
            crc = (uint16_t)((crc << 1) ^ 0x1021u);
        } else {
            crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

/* 计算当前版本设置记录的 CRC。 */
static uint16_t settings_crc(const SettingsRecord *r)
{
    const uint8_t *p;
    uint8_t i;
    uint16_t crc;

    p = (const uint8_t *)r;
    crc = 0xFFFFu;
    for (i = 0; i < (uint8_t)(sizeof(SettingsRecord) - sizeof(r->crc)); i++) {
        crc = settings_crc16_update(crc, p[i]);
    }
    return crc;
}

/* 兼容早期版本的轻量校验，避免升级后丢失旧设置。 */
static uint16_t settings_crc_legacy(const SettingsRecord *r)
{
    uint16_t crc;

    crc = 0xA55Au;
    crc ^= r->magic;
    crc = (uint16_t)((crc << 1) | (crc >> 15));
    crc ^= r->seq;
    crc = (uint16_t)((crc << 1) | (crc >> 15));
    crc ^= r->sample_interval;
    crc = (uint16_t)((crc << 1) | (crc >> 15));
    crc ^= r->alarm_duration;
    crc = (uint16_t)((crc << 1) | (crc >> 15));
    crc ^= r->hourglass_seconds;
    crc = (uint16_t)((crc << 1) | (crc >> 15));
    crc ^= r->reserved;
    crc = (uint16_t)((crc << 1) | (crc >> 15));
    crc ^= (uint16_t)r->threshold_t10;
    crc = (uint16_t)((crc << 1) | (crc >> 15));
    crc ^= r->storage_limit;
    return crc;
}

/* 校验一条设置记录的标识、范围和 CRC。 */
static uint8_t settings_record_valid(const SettingsRecord *r)
{
    if (r->magic != SETTINGS_MAGIC) {
        return 0;
    }
    if (r->threshold_t10 < ALERT_THRESHOLD_MIN_T10 || r->threshold_t10 > ALERT_THRESHOLD_MAX_T10) {
        return 0;
    }
    if (r->sample_interval < SAMPLE_INTERVAL_MIN_SECONDS || r->sample_interval > SAMPLE_INTERVAL_MAX_SECONDS) {
        return 0;
    }
    if (r->alarm_duration < ALARM_DURATION_MIN_SECONDS || r->alarm_duration > ALARM_DURATION_MAX_SECONDS) {
        return 0;
    }
    if (r->hourglass_seconds < HOURGLASS_MIN_SECONDS || r->hourglass_seconds > HOURGLASS_MAX_SECONDS) {
        return 0;
    }
    if (r->storage_limit < STORAGE_LIMIT_MIN || r->storage_limit > STORAGE_LIMIT_MAX) {
        return 0;
    }
    return (uint8_t)(settings_crc(r) == r->crc || settings_crc_legacy(r) == r->crc);
}

/* 把内部 Flash 记录转换成应用设置值。 */
static void settings_record_to_value(const SettingsRecord *r, SettingsStoreValue *value)
{
    value->sample_interval = r->sample_interval;
    value->alarm_duration = r->alarm_duration;
    value->hourglass_seconds = r->hourglass_seconds;
    value->threshold_t10 = r->threshold_t10;
    value->storage_limit = r->storage_limit;
}

/* 把应用设置值转换成带序号、magic 和 CRC 的 Flash 记录。 */
static void settings_value_to_record(const SettingsStoreValue *value, SettingsRecord *r)
{
    r->magic = SETTINGS_MAGIC;
    r->seq = g_settings_next_seq;
    r->sample_interval = value->sample_interval;
    r->alarm_duration = value->alarm_duration;
    r->hourglass_seconds = value->hourglass_seconds;
    r->reserved = 0;
    r->threshold_t10 = value->threshold_t10;
    r->storage_limit = value->storage_limit;
    r->crc = settings_crc(r);
}

/* 擦除保存设置用的 Info Flash 段，记录区写满后从头重新写。 */
static uint8_t settings_erase_segment(void)
{
    if (!settings_flash_wait_ready()) {
        return 0;
    }
    __disable_interrupt();
    FCTL3 = FWKEY;
    FCTL1 = FWKEY | ERASE;
    *(volatile uint16_t *)SETTINGS_FLASH_START = 0;
    __enable_interrupt();
    if (!settings_flash_wait_ready()) {
        __disable_interrupt();
        FCTL1 = FWKEY;
        FCTL3 = FWKEY | LOCK;
        __enable_interrupt();
        return 0;
    }
    __disable_interrupt();
    FCTL1 = FWKEY;
    FCTL3 = FWKEY | LOCK;
    __enable_interrupt();
    return 1;
}

/* 向 Info Flash 指定槽位写入一条设置记录。 */
static uint8_t settings_write_record(uint16_t index, const SettingsRecord *r)
{
    const uint16_t *src;
    volatile uint16_t *dst;
    uint8_t words;
    uint8_t i;

    src = (const uint16_t *)r;
    dst = (volatile uint16_t *)(SETTINGS_FLASH_START + index * sizeof(SettingsRecord));
    words = (uint8_t)(sizeof(SettingsRecord) / 2u);

    if (!settings_flash_wait_ready()) {
        return 0;
    }
    __disable_interrupt();
    FCTL3 = FWKEY;
    FCTL1 = FWKEY | WRT;
    for (i = 0; i < words; i++) {
        dst[i] = src[i];
    }
    __enable_interrupt();
    if (!settings_flash_wait_ready()) {
        __disable_interrupt();
        FCTL1 = FWKEY;
        FCTL3 = FWKEY | LOCK;
        __enable_interrupt();
        return 0;
    }
    __disable_interrupt();
    FCTL1 = FWKEY;
    FCTL3 = FWKEY | LOCK;
    __enable_interrupt();
    return 1;
}

uint8_t settings_store_load(SettingsStoreValue *value)
{
    uint16_t i;
    uint8_t found;
    const SettingsRecord *r;

    if (value == 0) {
        return 0;
    }

    g_settings_next_index = 0;
    g_settings_next_seq = 0;
    found = 0;

    for (i = 0; i < SETTINGS_RECORD_COUNT; i++) {
        r = (const SettingsRecord *)(SETTINGS_FLASH_START + i * sizeof(SettingsRecord));
        if (r->magic == 0xFFFFu) {
            g_settings_next_index = i;
            return found;
        }
        if (!settings_record_valid(r)) {
            g_settings_next_index = SETTINGS_RECORD_COUNT;
            return found;
        }
        settings_record_to_value(r, value);
        found = 1;
        g_settings_next_index = (uint16_t)(i + 1u);
        g_settings_next_seq = (uint16_t)(r->seq + 1u);
    }
    return found;
}

uint8_t settings_store_save(const SettingsStoreValue *value)
{
    SettingsRecord r;

    if (value == 0) {
        return 0;
    }

    settings_value_to_record(value, &r);
    if (g_settings_next_index >= SETTINGS_RECORD_COUNT) {
        if (!settings_erase_segment()) {
            return 0;
        }
        g_settings_next_index = 0;
    }
    if (!settings_write_record(g_settings_next_index, &r)) {
        return 0;
    }
    g_settings_next_index++;
    g_settings_next_seq++;
    return 1;
}
