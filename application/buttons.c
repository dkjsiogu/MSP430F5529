#include "buttons.h"

#include "app_state.h"
#include "app_resources.h"
#include "board.h"
#include "captouch.h"
#include "epaper.h"

typedef enum {
    BUTTON_ACTION_PRIMARY = 0u,     /* 主动作：主界面进 GIF，S1 触发。 */
    BUTTON_ACTION_SECONDARY,        /* 次动作：主界面进文本，S2 触发。 */
    BUTTON_ACTION_UP,               /* 向上动作：前一项/上一页/增加，Pad1 触发。 */
    BUTTON_ACTION_DOWN,             /* 向下动作：后一项/下一页/减少，Pad2 触发。 */
    BUTTON_ACTION_BACK,             /* 返回动作：进入设置或返回主界面，S3 触发。 */
    BUTTON_ACTION_CONFIRM,          /* 确认动作：确认设置或手动刷新，S4 触发。 */
    BUTTON_ACTION_COUNT             /* 语义动作数量。 */
} ButtonAction;

typedef enum {
    BUTTON_INPUT_GPIO_PORT1 = 0u,   /* P1 端口上的普通机械按键输入。 */
    BUTTON_INPUT_GPIO_PORT2,        /* P2 端口上的普通机械按键输入。 */
    BUTTON_INPUT_TOUCH              /* Comparator B 电容触摸输入。 */
} ButtonInputKind;

typedef struct {
    ButtonInputKind kind;           /* 输入源类型。 */
    uint8_t id;                     /* GPIO 位或触摸通道编号。 */
    ButtonAction action;            /* 该输入源触发的应用语义动作。 */
} ButtonInputBinding;

typedef void (*ButtonActionHandler)(const TempSample *last_sample, uint8_t has_sample);

#define BUTTON_ACTION_BIT(action)   ((uint8_t)(1u << (uint8_t)(action))) /* 把语义动作编号转换成待处理位。 */

#define BUTTON_MODE_MAIN            0u   /* 主温度界面按键模式。 */
#define BUTTON_MODE_SETTINGS_SELECT 1u   /* 设置界面参数选择模式。 */
#define BUTTON_MODE_SETTINGS_EDIT   2u   /* 设置界面参数编辑模式。 */
#define BUTTON_MODE_HISTORY         3u   /* 历史记录播放界面按键模式。 */
#define BUTTON_MODE_GIF             4u   /* 全屏 GIF 动画播放界面按键模式。 */
#define BUTTON_MODE_TEXT            5u   /* SD 卡文本阅读界面按键模式。 */

#define SETTINGS_ITEM_SAMPLE        0u   /* 设置项：定时采样间隔。 */
#define SETTINGS_ITEM_ALARM_TEMP    1u   /* 设置项：报警温度阈值。 */
#define SETTINGS_ITEM_STORAGE       2u   /* 设置项：Flash 历史记录保存条数。 */
#define SETTINGS_ITEM_ALARM_TIME    3u   /* 设置项：蜂鸣器报警持续时间。 */
#define SETTINGS_ITEM_HOURGLASS     4u   /* 设置项：沙漏动画周期和 TMP 平均温度窗口。 */
#define SETTINGS_ITEM_COUNT         5u   /* 设置项总数，用于选择项循环。 */

static const ButtonInputBinding g_button_input_bindings[] = {
    { BUTTON_INPUT_GPIO_PORT1, BUTTON_S1_BIT, BUTTON_ACTION_PRIMARY },
    { BUTTON_INPUT_GPIO_PORT1, BUTTON_S2_BIT, BUTTON_ACTION_SECONDARY },
    { BUTTON_INPUT_TOUCH, CAP_TOUCH_PAD1_CHANNEL, BUTTON_ACTION_UP },
    { BUTTON_INPUT_TOUCH, CAP_TOUCH_PAD2_CHANNEL, BUTTON_ACTION_DOWN },
    { BUTTON_INPUT_GPIO_PORT2, BUTTON_S3_BIT, BUTTON_ACTION_BACK },
    { BUTTON_INPUT_GPIO_PORT2, BUTTON_S4_BIT, BUTTON_ACTION_CONFIRM }
};

#define BUTTON_INPUT_BINDING_COUNT  ((uint8_t)(sizeof(g_button_input_bindings) / sizeof(g_button_input_bindings[0])))
#define BUTTON_GPIO_PORT_COUNT      2u
#define BUTTON_TOUCH_BIT(channel)   ((uint8_t)(1u << (uint8_t)(channel)))

static volatile uint8_t g_button_irq_bits[BUTTON_GPIO_PORT_COUNT] = {0, 0};
static uint8_t g_button_gpio_mask[BUTTON_GPIO_PORT_COUNT] = {0, 0};
static uint8_t g_button_last_pressed[BUTTON_GPIO_PORT_COUNT] = {0, 0};
static uint8_t g_touch_last_pressed = 0;
static uint8_t g_button_action_guard_seen = 0;
static uint16_t g_button_action_last_tick[BUTTON_ACTION_COUNT];
static uint8_t g_button_mode = BUTTON_MODE_MAIN;
static uint8_t g_settings_item = SETTINGS_ITEM_SAMPLE;
static ButtonsIsrWakeHook g_buttons_wake_hook = 0;

/* 从输入映射表预计算两个 GPIO 端口的机械按键位掩码。 */
static void buttons_build_gpio_masks(void)
{
    uint8_t i;

    g_button_gpio_mask[BUTTON_INPUT_GPIO_PORT1] = 0;
    g_button_gpio_mask[BUTTON_INPUT_GPIO_PORT2] = 0;
    for (i = 0; i < BUTTON_INPUT_BINDING_COUNT; i++) {
        if (g_button_input_bindings[i].kind == BUTTON_INPUT_GPIO_PORT1 ||
            g_button_input_bindings[i].kind == BUTTON_INPUT_GPIO_PORT2) {
            g_button_gpio_mask[g_button_input_bindings[i].kind] |= g_button_input_bindings[i].id;
        }
    }
}

/* 返回某个 GPIO 输入端口的机械按键位掩码。 */
static uint8_t buttons_gpio_port_mask(ButtonInputKind kind)
{
    if (kind == BUTTON_INPUT_GPIO_PORT1 || kind == BUTTON_INPUT_GPIO_PORT2) {
        return g_button_gpio_mask[kind];
    }
    return 0;
}

/* 读取指定 GPIO 输入端口的稳定低电平按下状态。 */
static uint8_t buttons_read_gpio_pressed(ButtonInputKind kind)
{
    uint8_t mask;

    mask = buttons_gpio_port_mask(kind);
    if (kind == BUTTON_INPUT_GPIO_PORT1) {
        return (uint8_t)((~BUTTON_PORT_IN) & mask);
    }
    if (kind == BUTTON_INPUT_GPIO_PORT2) {
        return (uint8_t)((~BUTTON2_PORT_IN) & mask);
    }
    return 0;
}

/* 按输入映射表初始化一个 GPIO 端口的上拉和下降沿中断。 */
static void buttons_init_gpio_port(ButtonInputKind kind)
{
    uint8_t mask;

    mask = buttons_gpio_port_mask(kind);
    if (mask == 0) {
        return;
    }

    if (kind == BUTTON_INPUT_GPIO_PORT1) {
        BUTTON_PORT_SEL &= ~mask;
        BUTTON_PORT_DIR &= ~mask;
        BUTTON_PORT_OUT |= mask;
        BUTTON_PORT_REN |= mask;
        BUTTON_PORT_IES |= mask;
        BUTTON_PORT_IFG &= ~mask;
        BUTTON_PORT_IE |= mask;
    } else if (kind == BUTTON_INPUT_GPIO_PORT2) {
        BUTTON2_PORT_SEL &= ~mask;
        BUTTON2_PORT_DIR &= ~mask;
        BUTTON2_PORT_OUT |= mask;
        BUTTON2_PORT_REN |= mask;
        BUTTON2_PORT_IES |= mask;
        BUTTON2_PORT_IFG &= ~mask;
        BUTTON2_PORT_IE |= mask;
    }
}

/* 重新打开指定 GPIO 输入端口的中断，供去抖完成后恢复边沿触发。 */
static void buttons_rearm_gpio_port(ButtonInputKind kind)
{
    uint8_t mask;

    mask = buttons_gpio_port_mask(kind);
    if (mask == 0) {
        return;
    }

    if (kind == BUTTON_INPUT_GPIO_PORT1) {
        BUTTON_PORT_IFG &= ~mask;
        BUTTON_PORT_IE |= mask;
    } else if (kind == BUTTON_INPUT_GPIO_PORT2) {
        BUTTON2_PORT_IFG &= ~mask;
        BUTTON2_PORT_IE |= mask;
    }
}

void buttons_init(void)
{
    buttons_build_gpio_masks();
    buttons_init_gpio_port(BUTTON_INPUT_GPIO_PORT1);
    buttons_init_gpio_port(BUTTON_INPUT_GPIO_PORT2);
    captouch_init();
}

void buttons_set_wake_hook(ButtonsIsrWakeHook hook)
{
    g_buttons_wake_hook = hook;
}

/* 根据每个语义动作独立的保护时间过滤重复边沿，减少误触发。 */
static uint8_t button_action_allowed(uint8_t action)
{
    uint16_t now;
    uint8_t action_bit;

    if (action >= BUTTON_ACTION_COUNT) {
        return 0;
    }
    now = board_tick10();
    action_bit = BUTTON_ACTION_BIT(action);
    if ((g_button_action_guard_seen & action_bit) &&
        !board_tick10_elapsed(g_button_action_last_tick[action], BUTTON_REPEAT_GUARD_TICKS)) {
        return 0;
    }

    g_button_action_guard_seen |= action_bit;
    g_button_action_last_tick[action] = now;
    return 1;
}

/* 从端口中断和实时电平中提取一次稳定的语义动作集合。 */
static uint8_t buttons_take_actions(void)
{
    uint8_t irq_bits[BUTTON_GPIO_PORT_COUNT];
    uint8_t gpio_pressed[BUTTON_GPIO_PORT_COUNT];
    uint8_t new_gpio_pressed[BUTTON_GPIO_PORT_COUNT];
    uint8_t touch_pressed;
    uint8_t new_touch_pressed;
    uint8_t actions;
    uint8_t action_bit;
    uint8_t i;

    gpio_pressed[BUTTON_INPUT_GPIO_PORT1] = buttons_read_gpio_pressed(BUTTON_INPUT_GPIO_PORT1);
    gpio_pressed[BUTTON_INPUT_GPIO_PORT2] = buttons_read_gpio_pressed(BUTTON_INPUT_GPIO_PORT2);
    touch_pressed = captouch_read_pressed_mask();
    irq_bits[BUTTON_INPUT_GPIO_PORT1] = g_button_irq_bits[BUTTON_INPUT_GPIO_PORT1];
    irq_bits[BUTTON_INPUT_GPIO_PORT2] = g_button_irq_bits[BUTTON_INPUT_GPIO_PORT2];
    if (irq_bits[BUTTON_INPUT_GPIO_PORT1] == 0 && irq_bits[BUTTON_INPUT_GPIO_PORT2] == 0 &&
        gpio_pressed[BUTTON_INPUT_GPIO_PORT1] == g_button_last_pressed[BUTTON_INPUT_GPIO_PORT1] &&
        gpio_pressed[BUTTON_INPUT_GPIO_PORT2] == g_button_last_pressed[BUTTON_INPUT_GPIO_PORT2] &&
        touch_pressed == g_touch_last_pressed) {
        return 0;
    }

    delay_ms(BUTTON_DEBOUNCE_MS);

    gpio_pressed[BUTTON_INPUT_GPIO_PORT1] = buttons_read_gpio_pressed(BUTTON_INPUT_GPIO_PORT1);
    gpio_pressed[BUTTON_INPUT_GPIO_PORT2] = buttons_read_gpio_pressed(BUTTON_INPUT_GPIO_PORT2);
    touch_pressed = captouch_read_pressed_mask();
    new_gpio_pressed[BUTTON_INPUT_GPIO_PORT1] = (uint8_t)(gpio_pressed[BUTTON_INPUT_GPIO_PORT1] &
                                                          (uint8_t)~g_button_last_pressed[BUTTON_INPUT_GPIO_PORT1]);
    new_gpio_pressed[BUTTON_INPUT_GPIO_PORT2] = (uint8_t)(gpio_pressed[BUTTON_INPUT_GPIO_PORT2] &
                                                          (uint8_t)~g_button_last_pressed[BUTTON_INPUT_GPIO_PORT2]);
    new_touch_pressed = (uint8_t)(touch_pressed & (uint8_t)~g_touch_last_pressed);
    actions = 0;
    for (i = 0; i < BUTTON_INPUT_BINDING_COUNT; i++) {
        if (g_button_input_bindings[i].kind == BUTTON_INPUT_GPIO_PORT1 &&
            (new_gpio_pressed[BUTTON_INPUT_GPIO_PORT1] & g_button_input_bindings[i].id) == 0) {
            continue;
        }
        if (g_button_input_bindings[i].kind == BUTTON_INPUT_GPIO_PORT2 &&
            (new_gpio_pressed[BUTTON_INPUT_GPIO_PORT2] & g_button_input_bindings[i].id) == 0) {
            continue;
        }
        if (g_button_input_bindings[i].kind == BUTTON_INPUT_TOUCH &&
            (new_touch_pressed & BUTTON_TOUCH_BIT(g_button_input_bindings[i].id)) == 0) {
            continue;
        }

        action_bit = BUTTON_ACTION_BIT(g_button_input_bindings[i].action);
        if ((actions & action_bit) ||
            button_action_allowed((uint8_t)g_button_input_bindings[i].action)) {
            actions |= action_bit;
        }
    }

    g_button_irq_bits[BUTTON_INPUT_GPIO_PORT1] = 0;
    g_button_irq_bits[BUTTON_INPUT_GPIO_PORT2] = 0;
    g_button_last_pressed[BUTTON_INPUT_GPIO_PORT1] = gpio_pressed[BUTTON_INPUT_GPIO_PORT1];
    g_button_last_pressed[BUTTON_INPUT_GPIO_PORT2] = gpio_pressed[BUTTON_INPUT_GPIO_PORT2];
    g_touch_last_pressed = touch_pressed;
    buttons_rearm_gpio_port(BUTTON_INPUT_GPIO_PORT1);
    buttons_rearm_gpio_port(BUTTON_INPUT_GPIO_PORT2);
    return actions;
}

#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
    uint8_t flags;
    uint8_t mask;

    mask = g_button_gpio_mask[BUTTON_INPUT_GPIO_PORT1];
    flags = (uint8_t)(BUTTON_PORT_IFG & mask);
    if (flags) {
        g_button_irq_bits[BUTTON_INPUT_GPIO_PORT1] |= flags;
        BUTTON_PORT_IE &= ~mask;
        BUTTON_PORT_IFG &= ~mask;
        if (g_buttons_wake_hook != 0) {
            g_buttons_wake_hook();
        }
        __bic_SR_register_on_exit(LPM0_bits);
    }
}

/* 根据当前温度样本刷新蜂鸣器报警状态。 */
static void buttons_apply_alarm_state(const TempSample *last_sample, uint8_t has_sample)
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
static void buttons_show_settings(void)
{
    epd_show_settings_page(g_settings_item, (uint8_t)(g_button_mode == BUTTON_MODE_SETTINGS_EDIT));
}

/* 从主界面进入设置选择模式，并显示第一个设置项。 */
static void buttons_enter_settings(void)
{
    g_button_mode = BUTTON_MODE_SETTINGS_SELECT;
    g_settings_item = SETTINGS_ITEM_SAMPLE;
    buttons_show_settings();
}

/* 从主界面进入 SD 卡文本阅读页面。 */
static void buttons_enter_text(void)
{
    g_button_mode = BUTTON_MODE_TEXT;
    (void)app_resources_reload();
    epd_show_text_reader();
}

/* 从主界面进入全屏 GIF 动画播放页面。 */
static void buttons_enter_gif(void)
{
    g_button_mode = BUTTON_MODE_GIF;
    (void)app_resources_reload();
    epd_show_gif_playback();
}

/* 退出设置或历史页面，保存设置并恢复主温度界面自动刷新。 */
static void buttons_return_main(void)
{
    g_button_mode = BUTTON_MODE_MAIN;
    app_flush_settings();
    epd_resume_auto();
    sample_timer_force_due();
}

/* 在设置选择模式中上下切换当前参数项。 */
static void buttons_move_setting(int8_t delta)
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
    buttons_show_settings();
}

/* 在设置编辑模式中根据方向调整当前参数，并刷新页面。 */
static void buttons_adjust_setting(const TempSample *last_sample, uint8_t has_sample, int8_t direction)
{
    switch (g_settings_item) {
    case SETTINGS_ITEM_SAMPLE:
        (void)app_adjust_sample_interval((int8_t)(direction * SAMPLE_INTERVAL_STEP_SECONDS));
        break;
    case SETTINGS_ITEM_ALARM_TEMP:
        (void)app_adjust_threshold_t10((int16_t)(direction * ALERT_THRESHOLD_STEP_T10));
        buttons_apply_alarm_state(last_sample, has_sample);
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
    buttons_show_settings();
}

/* 执行一次类似上电后的墨水屏全屏刷新，并强制重新采样。 */
static void buttons_manual_refresh(void)
{
    epd_full_refresh_once();
    sample_timer_force_due();
}

/* 在设置页的选择模式和编辑模式之间切换。 */
static void buttons_confirm_setting(void)
{
    if (g_button_mode == BUTTON_MODE_SETTINGS_SELECT) {
        g_button_mode = BUTTON_MODE_SETTINGS_EDIT;
    } else if (g_button_mode == BUTTON_MODE_SETTINGS_EDIT) {
        g_button_mode = BUTTON_MODE_SETTINGS_SELECT;
    }
    buttons_show_settings();
}

static void buttons_handle_primary(const TempSample *last_sample, uint8_t has_sample)
{
    if (g_button_mode == BUTTON_MODE_MAIN) {
        (void)last_sample;
        (void)has_sample;
        buttons_enter_gif();
    } else if (g_button_mode == BUTTON_MODE_HISTORY) {
        buttons_return_main();
    } else if (g_button_mode == BUTTON_MODE_GIF) {
        epd_gif_prev_asset();
    } else if (g_button_mode == BUTTON_MODE_TEXT) {
        epd_text_prev_page();
    } else if (g_button_mode == BUTTON_MODE_SETTINGS_SELECT) {
        buttons_move_setting(-1);
    } else if (g_button_mode == BUTTON_MODE_SETTINGS_EDIT) {
        buttons_adjust_setting(last_sample, has_sample, 1);
    } else {
        (void)last_sample;
        (void)has_sample;
    }
}

static void buttons_handle_secondary(const TempSample *last_sample, uint8_t has_sample)
{
    if (g_button_mode == BUTTON_MODE_MAIN) {
        (void)last_sample;
        (void)has_sample;
        buttons_enter_text();
    } else if (g_button_mode == BUTTON_MODE_GIF) {
        epd_gif_next_asset();
    } else if (g_button_mode == BUTTON_MODE_TEXT) {
        epd_text_next_page();
    } else if (g_button_mode == BUTTON_MODE_SETTINGS_SELECT) {
        buttons_move_setting(1);
    } else if (g_button_mode == BUTTON_MODE_SETTINGS_EDIT) {
        buttons_adjust_setting(last_sample, has_sample, -1);
    } else {
        (void)last_sample;
        (void)has_sample;
    }
}

static void buttons_handle_up(const TempSample *last_sample, uint8_t has_sample)
{
    if (g_button_mode == BUTTON_MODE_GIF) {
        (void)last_sample;
        (void)has_sample;
        epd_gif_prev_asset();
    } else if (g_button_mode == BUTTON_MODE_TEXT) {
        (void)last_sample;
        (void)has_sample;
        epd_text_prev_page();
    } else if (g_button_mode == BUTTON_MODE_SETTINGS_SELECT) {
        buttons_move_setting(-1);
    } else if (g_button_mode == BUTTON_MODE_SETTINGS_EDIT) {
        buttons_adjust_setting(last_sample, has_sample, 1);
    } else {
        (void)last_sample;
        (void)has_sample;
    }
}

static void buttons_handle_down(const TempSample *last_sample, uint8_t has_sample)
{
    if (g_button_mode == BUTTON_MODE_GIF) {
        (void)last_sample;
        (void)has_sample;
        epd_gif_next_asset();
    } else if (g_button_mode == BUTTON_MODE_TEXT) {
        (void)last_sample;
        (void)has_sample;
        epd_text_next_page();
    } else if (g_button_mode == BUTTON_MODE_SETTINGS_SELECT) {
        buttons_move_setting(1);
    } else if (g_button_mode == BUTTON_MODE_SETTINGS_EDIT) {
        buttons_adjust_setting(last_sample, has_sample, -1);
    } else {
        (void)last_sample;
        (void)has_sample;
    }
}

static void buttons_handle_back(const TempSample *last_sample, uint8_t has_sample)
{
    (void)last_sample;
    (void)has_sample;
    if (g_button_mode == BUTTON_MODE_MAIN) {
        buttons_enter_settings();
    } else {
        buttons_return_main();
    }
}

static void buttons_handle_confirm(const TempSample *last_sample, uint8_t has_sample)
{
    (void)last_sample;
    (void)has_sample;
    if (g_button_mode == BUTTON_MODE_MAIN) {
        buttons_manual_refresh();
    } else if (g_button_mode == BUTTON_MODE_SETTINGS_SELECT ||
               g_button_mode == BUTTON_MODE_SETTINGS_EDIT) {
        buttons_confirm_setting();
    } else {
        (void)last_sample;
        (void)has_sample;
    }
}

static const ButtonActionHandler button_action_handlers[BUTTON_ACTION_COUNT] = {
    buttons_handle_primary,
    buttons_handle_secondary,
    buttons_handle_up,
    buttons_handle_down,
    buttons_handle_back,
    buttons_handle_confirm
};

/* 调用一个语义动作处理器，物理输入和业务行为在这里完成解耦。 */
static void buttons_dispatch_action(uint8_t action, const TempSample *last_sample, uint8_t has_sample)
{
    if (action < BUTTON_ACTION_COUNT) {
        button_action_handlers[action](last_sample, has_sample);
    }
}

void buttons_task(const TempSample *last_sample, uint8_t has_sample)
{
    uint8_t actions;
    uint8_t action;

    actions = buttons_take_actions();
    for (action = 0; action < BUTTON_ACTION_COUNT; action++) {
        if (actions & BUTTON_ACTION_BIT(action)) {
            buttons_dispatch_action(action, last_sample, has_sample);
        }
    }
}

uint8_t buttons_pending(void)
{
    uint8_t gpio_pressed[BUTTON_GPIO_PORT_COUNT];
    uint8_t touch_pressed;

    gpio_pressed[BUTTON_INPUT_GPIO_PORT1] = buttons_read_gpio_pressed(BUTTON_INPUT_GPIO_PORT1);
    gpio_pressed[BUTTON_INPUT_GPIO_PORT2] = buttons_read_gpio_pressed(BUTTON_INPUT_GPIO_PORT2);
    touch_pressed = captouch_read_pressed_mask();
    return (uint8_t)(g_button_irq_bits[BUTTON_INPUT_GPIO_PORT1] != 0 ||
                     g_button_irq_bits[BUTTON_INPUT_GPIO_PORT2] != 0 ||
                     (gpio_pressed[BUTTON_INPUT_GPIO_PORT1] &
                      (uint8_t)~g_button_last_pressed[BUTTON_INPUT_GPIO_PORT1]) != 0 ||
                     (gpio_pressed[BUTTON_INPUT_GPIO_PORT2] &
                      (uint8_t)~g_button_last_pressed[BUTTON_INPUT_GPIO_PORT2]) != 0 ||
                     (touch_pressed & (uint8_t)~g_touch_last_pressed) != 0);
}

#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void)
{
    uint8_t flags;
    uint8_t mask;

    mask = g_button_gpio_mask[BUTTON_INPUT_GPIO_PORT2];
    flags = (uint8_t)(BUTTON2_PORT_IFG & mask);
    if (flags) {
        g_button_irq_bits[BUTTON_INPUT_GPIO_PORT2] |= flags;
        BUTTON2_PORT_IE &= ~mask;
        BUTTON2_PORT_IFG &= ~mask;
        if (g_buttons_wake_hook != 0) {
            g_buttons_wake_hook();
        }
        __bic_SR_register_on_exit(LPM0_bits);
    }
}
