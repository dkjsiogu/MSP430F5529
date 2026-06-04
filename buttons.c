#include "buttons.h"

#include "app_state.h"
#include "board.h"
#include "epaper.h"

#define BUTTON_EVENT_S1             0x01u
#define BUTTON_EVENT_S2             0x02u
#define BUTTON_EVENT_S3             0x04u
#define BUTTON_EVENT_S4             0x08u

#define BUTTON_MODE_MAIN            0u
#define BUTTON_MODE_SETTINGS_SELECT 1u
#define BUTTON_MODE_SETTINGS_EDIT   2u
#define BUTTON_MODE_HISTORY         3u

#define SETTINGS_ITEM_SAMPLE        0u
#define SETTINGS_ITEM_ALARM_TEMP    1u
#define SETTINGS_ITEM_STORAGE       2u
#define SETTINGS_ITEM_ALARM_TIME    3u
#define SETTINGS_ITEM_COUNT         4u

#define BUTTON1_BITS                (BUTTON_S1_BIT | BUTTON_S2_BIT)
#define BUTTON2_BITS                (BUTTON_S3_BIT | BUTTON_S4_BIT)

static volatile uint8_t g_button1_irq_bits = 0;
static volatile uint8_t g_button2_irq_bits = 0;
static uint8_t g_button1_last_pressed = 0;
static uint8_t g_button2_last_pressed = 0;
static uint8_t g_button_guard_seen = 0;
static uint16_t g_button_s1_last_tick = 0;
static uint16_t g_button_s2_last_tick = 0;
static uint16_t g_button_s3_last_tick = 0;
static uint16_t g_button_s4_last_tick = 0;
static uint8_t g_button_mode = BUTTON_MODE_MAIN;
static uint8_t g_settings_item = SETTINGS_ITEM_SAMPLE;

void buttons_init(void)
{
    BUTTON_PORT_SEL &= ~BUTTON1_BITS;
    BUTTON_PORT_DIR &= ~BUTTON1_BITS;
    BUTTON_PORT_OUT |= BUTTON1_BITS;
    BUTTON_PORT_REN |= BUTTON1_BITS;

    BUTTON_PORT_IES |= BUTTON1_BITS;
    BUTTON_PORT_IFG &= ~BUTTON1_BITS;
    BUTTON_PORT_IE |= BUTTON1_BITS;

    BUTTON2_PORT_SEL &= ~BUTTON2_BITS;
    BUTTON2_PORT_DIR &= ~BUTTON2_BITS;
    BUTTON2_PORT_OUT |= BUTTON2_BITS;
    BUTTON2_PORT_REN |= BUTTON2_BITS;

    BUTTON2_PORT_IES |= BUTTON2_BITS;
    BUTTON2_PORT_IFG &= ~BUTTON2_BITS;
    BUTTON2_PORT_IE |= BUTTON2_BITS;
}

static uint8_t button_event_allowed(uint8_t event)
{
    uint16_t now;
    uint16_t *last_tick;

    now = board_tick10();
    switch (event) {
    case BUTTON_EVENT_S1:
        last_tick = &g_button_s1_last_tick;
        break;
    case BUTTON_EVENT_S2:
        last_tick = &g_button_s2_last_tick;
        break;
    case BUTTON_EVENT_S3:
        last_tick = &g_button_s3_last_tick;
        break;
    default:
        last_tick = &g_button_s4_last_tick;
        break;
    }

    if ((g_button_guard_seen & event) &&
        !board_tick10_elapsed(*last_tick, BUTTON_REPEAT_GUARD_TICKS)) {
        return 0;
    }

    g_button_guard_seen |= event;
    *last_tick = now;
    return 1;
}

static uint8_t buttons_take_events(void)
{
    uint8_t irq1_bits;
    uint8_t irq2_bits;
    uint8_t pressed1;
    uint8_t pressed2;
    uint8_t new1_pressed;
    uint8_t new2_pressed;
    uint8_t events;

    pressed1 = (uint8_t)((~BUTTON_PORT_IN) & BUTTON1_BITS);
    pressed2 = (uint8_t)((~BUTTON2_PORT_IN) & BUTTON2_BITS);
    irq1_bits = g_button1_irq_bits;
    irq2_bits = g_button2_irq_bits;
    if (irq1_bits == 0 && irq2_bits == 0 &&
        pressed1 == g_button1_last_pressed &&
        pressed2 == g_button2_last_pressed) {
        return 0;
    }

    delay_ms(BUTTON_DEBOUNCE_MS);

    pressed1 = (uint8_t)((~BUTTON_PORT_IN) & BUTTON1_BITS);
    pressed2 = (uint8_t)((~BUTTON2_PORT_IN) & BUTTON2_BITS);
    new1_pressed = (uint8_t)(pressed1 & (uint8_t)~g_button1_last_pressed);
    new2_pressed = (uint8_t)(pressed2 & (uint8_t)~g_button2_last_pressed);
    events = 0;
    if ((new1_pressed & BUTTON_S1_BIT) && button_event_allowed(BUTTON_EVENT_S1)) {
        events |= BUTTON_EVENT_S1;
    }
    if ((new1_pressed & BUTTON_S2_BIT) && button_event_allowed(BUTTON_EVENT_S2)) {
        events |= BUTTON_EVENT_S2;
    }
    if ((new2_pressed & BUTTON_S3_BIT) && button_event_allowed(BUTTON_EVENT_S3)) {
        events |= BUTTON_EVENT_S3;
    }
    if ((new2_pressed & BUTTON_S4_BIT) && button_event_allowed(BUTTON_EVENT_S4)) {
        events |= BUTTON_EVENT_S4;
    }

    g_button1_irq_bits = 0;
    g_button2_irq_bits = 0;
    g_button1_last_pressed = pressed1;
    g_button2_last_pressed = pressed2;
    BUTTON_PORT_IFG &= ~BUTTON1_BITS;
    BUTTON2_PORT_IFG &= ~BUTTON2_BITS;
    BUTTON_PORT_IE |= BUTTON1_BITS;
    BUTTON2_PORT_IE |= BUTTON2_BITS;
    return events;
}

#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
    uint8_t flags;

    flags = (uint8_t)(BUTTON_PORT_IFG & BUTTON1_BITS);
    if (flags) {
        g_button1_irq_bits |= flags;
        BUTTON_PORT_IE &= ~BUTTON1_BITS;
        BUTTON_PORT_IFG &= ~BUTTON1_BITS;
        __bic_SR_register_on_exit(LPM0_bits);
    }
}

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

static void buttons_show_settings(void)
{
    epd_show_settings_page(g_settings_item, (uint8_t)(g_button_mode == BUTTON_MODE_SETTINGS_EDIT));
}

static void buttons_enter_settings(void)
{
    g_button_mode = BUTTON_MODE_SETTINGS_SELECT;
    g_settings_item = SETTINGS_ITEM_SAMPLE;
    buttons_show_settings();
}

static void buttons_enter_history(void)
{
    g_button_mode = BUTTON_MODE_HISTORY;
    epd_show_history_playback();
}

static void buttons_return_main(void)
{
    g_button_mode = BUTTON_MODE_MAIN;
    app_flush_settings();
    epd_resume_auto();
    sample_timer_force_due();
}

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
    default:
        break;
    }
    buttons_show_settings();
}

static void buttons_manual_refresh(void)
{
    epd_full_refresh_once();
    sample_timer_force_due();
}

static void buttons_confirm_setting(void)
{
    if (g_button_mode == BUTTON_MODE_SETTINGS_SELECT) {
        g_button_mode = BUTTON_MODE_SETTINGS_EDIT;
    } else if (g_button_mode == BUTTON_MODE_SETTINGS_EDIT) {
        g_button_mode = BUTTON_MODE_SETTINGS_SELECT;
    }
    buttons_show_settings();
}

void buttons_action_s1(const TempSample *last_sample, uint8_t has_sample)
{
    if (g_button_mode == BUTTON_MODE_HISTORY) {
        buttons_return_main();
    } else if (g_button_mode == BUTTON_MODE_SETTINGS_SELECT) {
        buttons_move_setting(-1);
    } else if (g_button_mode == BUTTON_MODE_SETTINGS_EDIT) {
        buttons_adjust_setting(last_sample, has_sample, 1);
    } else {
        (void)last_sample;
        (void)has_sample;
    }
}

void buttons_action_s2(const TempSample *last_sample, uint8_t has_sample)
{
    if (g_button_mode == BUTTON_MODE_MAIN) {
        (void)last_sample;
        (void)has_sample;
        buttons_enter_history();
    } else if (g_button_mode == BUTTON_MODE_SETTINGS_SELECT) {
        buttons_move_setting(1);
    } else if (g_button_mode == BUTTON_MODE_SETTINGS_EDIT) {
        buttons_adjust_setting(last_sample, has_sample, -1);
    } else {
        (void)last_sample;
        (void)has_sample;
    }
}

void buttons_action_s3(const TempSample *last_sample, uint8_t has_sample)
{
    (void)last_sample;
    (void)has_sample;
    if (g_button_mode == BUTTON_MODE_MAIN) {
        buttons_enter_settings();
    } else {
        buttons_return_main();
    }
}

void buttons_action_s4(const TempSample *last_sample, uint8_t has_sample)
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

void buttons_task(const TempSample *last_sample, uint8_t has_sample)
{
    uint8_t events;

    events = buttons_take_events();
    if (events & BUTTON_EVENT_S1) {
        buttons_action_s1(last_sample, has_sample);
    }
    if (events & BUTTON_EVENT_S2) {
        buttons_action_s2(last_sample, has_sample);
    }
    if (events & BUTTON_EVENT_S3) {
        buttons_action_s3(last_sample, has_sample);
    }
    if (events & BUTTON_EVENT_S4) {
        buttons_action_s4(last_sample, has_sample);
    }
}

uint8_t buttons_pending(void)
{
    uint8_t pressed1;
    uint8_t pressed2;

    pressed1 = (uint8_t)((~BUTTON_PORT_IN) & BUTTON1_BITS);
    pressed2 = (uint8_t)((~BUTTON2_PORT_IN) & BUTTON2_BITS);
    return (uint8_t)(g_button1_irq_bits != 0 ||
                     g_button2_irq_bits != 0 ||
                     (pressed1 & (uint8_t)~g_button1_last_pressed) != 0 ||
                     (pressed2 & (uint8_t)~g_button2_last_pressed) != 0);
}

#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void)
{
    uint8_t flags;

    flags = (uint8_t)(BUTTON2_PORT_IFG & BUTTON2_BITS);
    if (flags) {
        g_button2_irq_bits |= flags;
        BUTTON2_PORT_IE &= ~BUTTON2_BITS;
        BUTTON2_PORT_IFG &= ~BUTTON2_BITS;
        __bic_SR_register_on_exit(LPM0_bits);
    }
}
