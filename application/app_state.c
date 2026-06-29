#include "app_state.h"

#include "app_config.h"
#include "board.h"
#include "settings_store.h"

static uint8_t g_sample_interval = SAMPLE_INTERVAL_SECONDS;
static uint8_t g_alarm_duration = ALARM_DURATION_SECONDS;
static uint8_t g_hourglass_seconds = HOURGLASS_SECONDS;
static int16_t g_threshold_t10 = ALERT_THRESHOLD_T10;
static uint16_t g_storage_limit = STORAGE_LIMIT_DEFAULT;
static uint8_t g_settings_save_pending = 0;
static uint16_t g_settings_save_request_tick = 0;
static uint8_t g_collection_running = 1;                    /* 采集运行状态：上电默认运行，S1 启动 / S2 停止。 */

/* 将设置值限制在应用允许范围内，避免写入越界配置。 */
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

/* 恢复默认设置，后续再用持久化值覆盖。 */
static void app_state_load_defaults(void)
{
    g_sample_interval = SAMPLE_INTERVAL_SECONDS;
    g_alarm_duration = ALARM_DURATION_SECONDS;
    g_hourglass_seconds = HOURGLASS_SECONDS;
    g_threshold_t10 = ALERT_THRESHOLD_T10;
    g_storage_limit = STORAGE_LIMIT_DEFAULT;
    g_settings_save_pending = 0;
    g_settings_save_request_tick = 0;
}

/* 将存储后端读出的值应用到运行状态。 */
static void app_state_apply_settings(const SettingsStoreValue *value)
{
    g_sample_interval = value->sample_interval;
    g_alarm_duration = value->alarm_duration;
    g_hourglass_seconds = value->hourglass_seconds;
    g_threshold_t10 = value->threshold_t10;
    g_storage_limit = value->storage_limit;
}

/* 生成准备写入存储后端的当前设置快照。 */
static void app_state_snapshot_settings(SettingsStoreValue *value)
{
    value->sample_interval = g_sample_interval;
    value->alarm_duration = g_alarm_duration;
    value->hourglass_seconds = g_hourglass_seconds;
    value->threshold_t10 = g_threshold_t10;
    value->storage_limit = g_storage_limit;
}

/* 标记设置需要稍后保存，避开连续按键时频繁擦写 Flash。 */
static void settings_mark_save_pending(void)
{
    g_settings_save_pending = 1;
    g_settings_save_request_tick = board_tick10();
}

void app_state_init(void)
{
    SettingsStoreValue value;

    app_state_load_defaults();
    if (settings_store_load(&value)) {
        app_state_apply_settings(&value);
    }
}

void app_save_settings(void)
{
    SettingsStoreValue value;

    if (!g_settings_save_pending) {
        return;
    }

    app_state_snapshot_settings(&value);
    if (settings_store_save(&value)) {
        g_settings_save_pending = 0;
    }
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

uint8_t app_hourglass_seconds(void)
{
    return g_hourglass_seconds;
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

uint8_t app_adjust_hourglass_seconds(int8_t delta_seconds)
{
    uint8_t new_seconds;
    long value;

    value = (long)g_hourglass_seconds + (long)delta_seconds;
    value = clamp_long(value, HOURGLASS_MIN_SECONDS, HOURGLASS_MAX_SECONDS);
    new_seconds = (uint8_t)value;
    if (new_seconds != g_hourglass_seconds) {
        g_hourglass_seconds = new_seconds;
        settings_mark_save_pending();
    }
    return g_hourglass_seconds;
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

uint8_t app_collection_running(void)
{
    return g_collection_running;
}

void app_set_collection_running(uint8_t running)
{
    g_collection_running = running ? 1u : 0u;
}
