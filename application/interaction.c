#include "interaction.h"

#include "app_config.h"
#include "app_state.h"
#include "app_resources.h"
#include "board.h"
#include "epaper.h"
#include "input.h"

typedef void (*InputEventHandler)(const TempSample *last_sample, uint8_t has_sample);

#define INTERACTION_MODE_MAIN            0u   /* 主温度界面按键模式。 */
#define INTERACTION_MODE_SETTINGS_SELECT 1u   /* 设置界面参数选择模式。 */
#define INTERACTION_MODE_SETTINGS_EDIT   2u   /* 设置界面参数编辑模式。 */
#define INTERACTION_MODE_HISTORY         3u   /* 历史记录播放界面按键模式。 */
#define INTERACTION_MODE_GIF             4u   /* 全屏 GIF 动画播放界面按键模式。 */
#define INTERACTION_MODE_TEXT            5u   /* SD 卡文本阅读界面按键模式。 */

#define SETTINGS_ITEM_SAMPLE        0u   /* 设置项：定时采样间隔。 */
#define SETTINGS_ITEM_ALARM_TEMP    1u   /* 设置项：报警温度阈值。 */
#define SETTINGS_ITEM_STORAGE       2u   /* 设置项：Flash 历史记录保存条数。 */
#define SETTINGS_ITEM_ALARM_TIME    3u   /* 设置项：蜂鸣器报警持续时间。 */
#define SETTINGS_ITEM_HOURGLASS     4u   /* 设置项：沙漏动画周期和 TMP 平均温度窗口。 */
#define SETTINGS_ITEM_COUNT         5u   /* 设置项总数，用于选择项循环。 */

static uint8_t g_interaction_mode = INTERACTION_MODE_MAIN;
static uint8_t g_settings_item = SETTINGS_ITEM_SAMPLE;

/* 根据当前温度样本刷新蜂鸣器报警状态。 */
static void interaction_apply_alarm_state(const TempSample *last_sample, uint8_t has_sample)
{
    if (has_sample && last_sample != 0) {
        if (sample_over_threshold(last_sample)) {
            buzzer_alert_for(app_alarm_duration_seconds());
        } else {
            buzzer_set(0);
        }
    }
}

/* 按当前选择项和编辑状态刷新设置页面。 */
static void interaction_show_settings(void)
{
    epd_show_settings_page(g_settings_item, (uint8_t)(g_interaction_mode == INTERACTION_MODE_SETTINGS_EDIT));
}

/* 从主界面进入设置选择模式，并显示第一个设置项。 */
static void interaction_enter_settings(void)
{
    g_interaction_mode = INTERACTION_MODE_SETTINGS_SELECT;
    g_settings_item = SETTINGS_ITEM_SAMPLE;
    interaction_show_settings();
}

/* 从主界面进入 SD 卡文本阅读页面。 */
static void interaction_enter_text(void)
{
    g_interaction_mode = INTERACTION_MODE_TEXT;
    (void)app_resources_reload();
    epd_show_text_reader();
}

/* 从主界面进入全屏 GIF 动画播放页面。 */
static void interaction_enter_gif(void)
{
    g_interaction_mode = INTERACTION_MODE_GIF;
    (void)app_resources_reload();
    epd_show_gif_playback();
}

/* 从主界面进入 Flash 历史记录滚动播放页。 */
static void interaction_enter_history(void)
{
    g_interaction_mode = INTERACTION_MODE_HISTORY;
    epd_show_history_playback();
}

/* 退出设置或历史页面，保存设置并恢复主温度界面自动刷新。 */
static void interaction_return_main(void)
{
    g_interaction_mode = INTERACTION_MODE_MAIN;
    app_flush_settings();
    epd_resume_auto();
    sample_timer_force_due();
}

/* 在设置选择模式中上下切换当前参数项。 */
static void interaction_move_setting(int8_t delta)
{
    if (delta < 0) {
        if (g_settings_item == 0) {
            g_settings_item = (uint8_t)(SETTINGS_ITEM_COUNT - 1u);
        } else {
            g_settings_item--;
        }
    } else if (delta > 0) {
        g_settings_item++;
        if (g_settings_item >= SETTINGS_ITEM_COUNT) {
            g_settings_item = 0;
        }
    }
    interaction_show_settings();
}

/* 在设置编辑模式中根据方向调整当前参数，并刷新页面。 */
static void interaction_adjust_setting(const TempSample *last_sample, uint8_t has_sample, int8_t direction)
{
    switch (g_settings_item) {
    case SETTINGS_ITEM_SAMPLE:
        (void)app_adjust_sample_interval((int8_t)(direction * SAMPLE_INTERVAL_STEP_SECONDS));
        break;
    case SETTINGS_ITEM_ALARM_TEMP:
        (void)app_adjust_threshold_t10((int16_t)(direction * ALERT_THRESHOLD_STEP_T10));
        interaction_apply_alarm_state(last_sample, has_sample);
        break;
    case SETTINGS_ITEM_STORAGE:
        (void)app_adjust_storage_limit((int16_t)(direction * STORAGE_LIMIT_STEP));
        break;
    case SETTINGS_ITEM_ALARM_TIME:
        (void)app_adjust_alarm_duration((int8_t)(direction * ALARM_DURATION_STEP_SECONDS));
        break;
    case SETTINGS_ITEM_HOURGLASS:
        (void)app_adjust_hourglass_seconds((int8_t)(direction * HOURGLASS_STEP_SECONDS));
        break;
    default:
        break;
    }
    interaction_show_settings();
}

/* 执行一次类似上电后的墨水屏全屏刷新，并强制重新采样。 */
/* 在设置页的选择模式和编辑模式之间切换。 */
static void interaction_confirm_setting(void)
{
    if (g_interaction_mode == INTERACTION_MODE_SETTINGS_SELECT) {
        g_interaction_mode = INTERACTION_MODE_SETTINGS_EDIT;
    } else if (g_interaction_mode == INTERACTION_MODE_SETTINGS_EDIT) {
        g_interaction_mode = INTERACTION_MODE_SETTINGS_SELECT;
    }
    interaction_show_settings();
}

static void interaction_handle_primary(const TempSample *last_sample, uint8_t has_sample)
{
    if (g_interaction_mode == INTERACTION_MODE_MAIN) {
        (void)last_sample;
        (void)has_sample;
        interaction_enter_gif();
    } else if (g_interaction_mode == INTERACTION_MODE_HISTORY) {
        interaction_return_main();
    } else if (g_interaction_mode == INTERACTION_MODE_GIF) {
        epd_gif_prev_asset();
    } else if (g_interaction_mode == INTERACTION_MODE_TEXT) {
        epd_text_prev_page();
    } else if (g_interaction_mode == INTERACTION_MODE_SETTINGS_SELECT) {
        interaction_move_setting(-1);
    } else if (g_interaction_mode == INTERACTION_MODE_SETTINGS_EDIT) {
        interaction_adjust_setting(last_sample, has_sample, 1);
    } else {
        (void)last_sample;
        (void)has_sample;
    }
}

static void interaction_handle_secondary(const TempSample *last_sample, uint8_t has_sample)
{
    if (g_interaction_mode == INTERACTION_MODE_MAIN) {
        (void)last_sample;
        (void)has_sample;
        interaction_enter_text();
    } else if (g_interaction_mode == INTERACTION_MODE_GIF) {
        epd_gif_next_asset();
    } else if (g_interaction_mode == INTERACTION_MODE_TEXT) {
        epd_text_next_page();
    } else if (g_interaction_mode == INTERACTION_MODE_SETTINGS_SELECT) {
        interaction_move_setting(1);
    } else if (g_interaction_mode == INTERACTION_MODE_SETTINGS_EDIT) {
        interaction_adjust_setting(last_sample, has_sample, -1);
    } else {
        (void)last_sample;
        (void)has_sample;
    }
}

static void interaction_handle_up(const TempSample *last_sample, uint8_t has_sample)
{
    if (g_interaction_mode == INTERACTION_MODE_GIF) {
        (void)last_sample;
        (void)has_sample;
        epd_gif_prev_asset();
    } else if (g_interaction_mode == INTERACTION_MODE_TEXT) {
        (void)last_sample;
        (void)has_sample;
        epd_text_prev_page();
    } else if (g_interaction_mode == INTERACTION_MODE_SETTINGS_SELECT) {
        interaction_move_setting(-1);
    } else if (g_interaction_mode == INTERACTION_MODE_SETTINGS_EDIT) {
        interaction_adjust_setting(last_sample, has_sample, 1);
    } else {
        (void)last_sample;
        (void)has_sample;
    }
}

static void interaction_handle_down(const TempSample *last_sample, uint8_t has_sample)
{
    if (g_interaction_mode == INTERACTION_MODE_GIF) {
        (void)last_sample;
        (void)has_sample;
        epd_gif_next_asset();
    } else if (g_interaction_mode == INTERACTION_MODE_TEXT) {
        (void)last_sample;
        (void)has_sample;
        epd_text_next_page();
    } else if (g_interaction_mode == INTERACTION_MODE_SETTINGS_SELECT) {
        interaction_move_setting(1);
    } else if (g_interaction_mode == INTERACTION_MODE_SETTINGS_EDIT) {
        interaction_adjust_setting(last_sample, has_sample, -1);
    } else {
        (void)last_sample;
        (void)has_sample;
    }
}

static void interaction_handle_back(const TempSample *last_sample, uint8_t has_sample)
{
    (void)last_sample;
    (void)has_sample;
    if (g_interaction_mode == INTERACTION_MODE_MAIN) {
        interaction_enter_settings();
    } else {
        interaction_return_main();
    }
}

static void interaction_handle_confirm(const TempSample *last_sample, uint8_t has_sample)
{
    (void)last_sample;
    (void)has_sample;
    if (g_interaction_mode == INTERACTION_MODE_MAIN) {
        interaction_enter_history();
    } else if (g_interaction_mode == INTERACTION_MODE_SETTINGS_SELECT ||
               g_interaction_mode == INTERACTION_MODE_SETTINGS_EDIT) {
        interaction_confirm_setting();
    } else {
        (void)last_sample;
        (void)has_sample;
    }
}

static const InputEventHandler input_event_handlers[INPUT_EVENT_COUNT] = {
    interaction_handle_primary,
    interaction_handle_secondary,
    interaction_handle_up,
    interaction_handle_down,
    interaction_handle_back,
    interaction_handle_confirm
};

/* 调用一个输入事件处理器，业务层只关心事件含义，不关心底层输入源。 */
static void interaction_dispatch_event(uint8_t event, const TempSample *last_sample, uint8_t has_sample)
{
    if (event < INPUT_EVENT_COUNT) {
        input_event_handlers[event](last_sample, has_sample);
    }
}

void interaction_task(const TempSample *last_sample, uint8_t has_sample)
{
    uint8_t events;
    uint8_t event;

    events = input_take_events();
    for (event = 0; event < INPUT_EVENT_COUNT; event++) {
        if (events & INPUT_EVENT_BIT(event)) {
            interaction_dispatch_event(event, last_sample, has_sample);
        }
    }
}

uint8_t interaction_pending(void)
{
    return input_pending();
}
