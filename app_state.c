#include "app_state.h"

#include "board.h"

typedef struct {
    uint16_t magic;
    uint16_t seq;
    uint8_t sample_interval;
    uint8_t alarm_duration;
    int16_t threshold_t10;
    uint16_t storage_limit;
    uint16_t crc;
} SettingsRecord;

#define SETTINGS_RECORD_COUNT       ((SETTINGS_FLASH_END - SETTINGS_FLASH_START) / sizeof(SettingsRecord)) /* Info Flash 设置区最多可保存的设置记录数。 */

static uint8_t g_sample_interval = SAMPLE_INTERVAL_SECONDS;
static uint8_t g_alarm_duration = ALARM_DURATION_SECONDS;
static int16_t g_threshold_t10 = ALERT_THRESHOLD_T10;
static uint16_t g_storage_limit = STORAGE_LIMIT_DEFAULT;
static uint16_t g_settings_next_index = 0;
static uint16_t g_settings_next_seq = 0;
static uint8_t g_settings_save_pending = 0;
static uint16_t g_settings_save_request_tick = 0;

/* 将设置项调整后的数值限制在允许范围内，避免写入越界配置。 */
static long clamp_long(long value, long min_value, long max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

/* 计算设置记录校验值，用来判断 Info Flash 中的配置是否完整有效。 */
static uint16_t settings_crc(const SettingsRecord *r)
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
    crc ^= (uint16_t)r->threshold_t10;
    crc = (uint16_t)((crc << 1) | (crc >> 15));
    crc ^= r->storage_limit;
    return crc;
}

/* 检查一条设置记录的标识、范围和 CRC 是否都合法。 */
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
    if (r->storage_limit < STORAGE_LIMIT_MIN || r->storage_limit > STORAGE_LIMIT_MAX) {
        return 0;
    }
    return (uint8_t)(settings_crc(r) == r->crc);
}

/* 擦除保存设置用的 Info Flash 段，记录区写满后从头重新写。 */
static void settings_erase_segment(void)
{
    while (FCTL3 & BUSY) {
        ;
    }
    FCTL3 = FWKEY;
    FCTL1 = FWKEY | ERASE;
    *(volatile uint16_t *)SETTINGS_FLASH_START = 0;
    while (FCTL3 & BUSY) {
        ;
    }
    FCTL1 = FWKEY;
    FCTL3 = FWKEY | LOCK;
}

/* 向 Info Flash 指定槽位写入一条设置记录。 */
static void settings_write_record(uint16_t index, const SettingsRecord *r)
{
    const uint16_t *src;
    volatile uint16_t *dst;
    uint8_t words;
    uint8_t i;

    src = (const uint16_t *)r;
    dst = (volatile uint16_t *)(SETTINGS_FLASH_START + index * sizeof(SettingsRecord));
    words = (uint8_t)(sizeof(SettingsRecord) / 2u);

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
}

/* 标记设置需要稍后保存，避开连续按键时频繁擦写 Flash。 */
static void settings_mark_save_pending(void)
{
    g_settings_save_pending = 1;
    g_settings_save_request_tick = board_tick10();
}

void app_state_init(void)
{
    uint16_t i;
    const SettingsRecord *r;

    g_threshold_t10 = ALERT_THRESHOLD_T10;
    g_sample_interval = SAMPLE_INTERVAL_SECONDS;
    g_alarm_duration = ALARM_DURATION_SECONDS;
    g_storage_limit = STORAGE_LIMIT_DEFAULT;
    g_settings_next_index = 0;
    g_settings_next_seq = 0;

    for (i = 0; i < SETTINGS_RECORD_COUNT; i++) {
        r = (const SettingsRecord *)(SETTINGS_FLASH_START + i * sizeof(SettingsRecord));
        if (r->magic == 0xFFFFu) {
            g_settings_next_index = i;
            return;
        }
        if (!settings_record_valid(r)) {
            g_settings_next_index = SETTINGS_RECORD_COUNT;
            return;
        }
        g_threshold_t10 = r->threshold_t10;
        g_sample_interval = r->sample_interval;
        g_alarm_duration = r->alarm_duration;
        g_storage_limit = r->storage_limit;
        g_settings_next_index = (uint16_t)(i + 1u);
        g_settings_next_seq = (uint16_t)(r->seq + 1u);
    }
}

void app_save_settings(void)
{
    SettingsRecord r;

    if (!g_settings_save_pending) {
        return;
    }

    r.magic = SETTINGS_MAGIC;
    r.seq = g_settings_next_seq++;
    r.sample_interval = g_sample_interval;
    r.alarm_duration = g_alarm_duration;
    r.threshold_t10 = g_threshold_t10;
    r.storage_limit = g_storage_limit;
    r.crc = settings_crc(&r);

    __disable_interrupt();
    if (g_settings_next_index >= SETTINGS_RECORD_COUNT) {
        settings_erase_segment();
        g_settings_next_index = 0;
    }
    settings_write_record(g_settings_next_index, &r);
    g_settings_next_index++;
    g_settings_save_pending = 0;
    __enable_interrupt();
}

void app_state_task(void)
{
    if (g_settings_save_pending &&
        board_tick10_elapsed(g_settings_save_request_tick, SETTINGS_SAVE_DELAY_TICKS)) {
        app_save_settings();
    }
}

void app_flush_settings(void)
{
    app_save_settings();
}

uint8_t app_sample_interval(void)
{
    return g_sample_interval;
}

uint8_t app_alarm_duration_seconds(void)
{
    return g_alarm_duration;
}

uint16_t app_storage_limit(void)
{
    return g_storage_limit;
}

int16_t app_threshold_t10(void)
{
    return g_threshold_t10;
}

uint8_t app_adjust_sample_interval(int8_t delta_seconds)
{
    uint8_t new_interval;
    long value;

    value = (long)g_sample_interval + (long)delta_seconds;
    value = clamp_long(value, SAMPLE_INTERVAL_MIN_SECONDS, SAMPLE_INTERVAL_MAX_SECONDS);
    new_interval = (uint8_t)value;
    if (new_interval != g_sample_interval) {
        g_sample_interval = new_interval;
        settings_mark_save_pending();
    }
    return g_sample_interval;
}

int16_t app_adjust_threshold_t10(int16_t delta_t10)
{
    int16_t new_threshold;
    long threshold;

    threshold = (long)g_threshold_t10 + (long)delta_t10;
    threshold = clamp_long(threshold, ALERT_THRESHOLD_MIN_T10, ALERT_THRESHOLD_MAX_T10);
    new_threshold = (int16_t)threshold;
    if (new_threshold != g_threshold_t10) {
        g_threshold_t10 = new_threshold;
        settings_mark_save_pending();
    }
    return g_threshold_t10;
}

uint16_t app_adjust_storage_limit(int16_t delta_records)
{
    uint16_t new_limit;
    long value;

    value = (long)g_storage_limit + (long)delta_records;
    value = clamp_long(value, STORAGE_LIMIT_MIN, STORAGE_LIMIT_MAX);
    new_limit = (uint16_t)value;
    if (new_limit != g_storage_limit) {
        g_storage_limit = new_limit;
        settings_mark_save_pending();
    }
    return g_storage_limit;
}

uint8_t app_adjust_alarm_duration(int8_t delta_seconds)
{
    uint8_t new_duration;
    long value;

    value = (long)g_alarm_duration + (long)delta_seconds;
    value = clamp_long(value, ALARM_DURATION_MIN_SECONDS, ALARM_DURATION_MAX_SECONDS);
    new_duration = (uint8_t)value;
    if (new_duration != g_alarm_duration) {
        g_alarm_duration = new_duration;
        settings_mark_save_pending();
    }
    return g_alarm_duration;
}

uint8_t temp_is_valid(int16_t t10)
{
    return (uint8_t)(t10 != INVALID_T10);
}

uint8_t sample_over_threshold(const TempSample *s)
{
    if (temp_is_valid(s->die_t10) && s->die_t10 >= app_threshold_t10()) {
        return 1;
    }
    if (temp_is_valid(s->ntc_t10) && s->ntc_t10 >= app_threshold_t10()) {
        return 1;
    }
    if (temp_is_valid(s->tmp_local_t10) && s->tmp_local_t10 >= app_threshold_t10()) {
        return 1;
    }
    return 0;
}
