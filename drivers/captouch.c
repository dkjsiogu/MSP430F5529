#include "captouch.h"

#include "platform_config.h"

static uint16_t g_captouch_baseline[CAP_TOUCH_CHANNEL_COUNT];
static uint8_t g_captouch_pressed_mask = 0;
static uint8_t g_captouch_press_samples[CAP_TOUCH_CHANNEL_COUNT];
static uint8_t g_captouch_release_samples[CAP_TOUCH_CHANNEL_COUNT];

/* 返回触摸通道对应的 P6 位。 */
static uint8_t captouch_channel_bit(uint8_t channel)
{
    return (uint8_t)(1u << channel);
}

/* 根据上电基线计算当前通道的触摸阈值，保留固定门限兜底。 */
static uint16_t captouch_press_threshold(uint8_t channel)
{
    uint16_t baseline;
    uint16_t adaptive;

    baseline = g_captouch_baseline[channel];
    adaptive = (uint16_t)(baseline + (baseline / CAP_TOUCH_THRESHOLD_RATIO_DIV) +
                          CAP_TOUCH_DELTA_MIN);
    if (adaptive < CAP_TOUCH_MIN_THRESHOLD) {
        adaptive = CAP_TOUCH_MIN_THRESHOLD;
    }
    return adaptive;
}

/* 释放门限低于按下门限，形成回差，防止触摸临界值反复触发。 */
static uint16_t captouch_release_threshold(uint8_t channel)
{
    uint16_t baseline;
    uint16_t press_threshold;
    uint16_t release_threshold;

    baseline = g_captouch_baseline[channel];
    press_threshold = captouch_press_threshold(channel);
    if (press_threshold > (uint16_t)(baseline + CAP_TOUCH_RELEASE_HYSTERESIS)) {
        release_threshold = (uint16_t)(press_threshold - CAP_TOUCH_RELEASE_HYSTERESIS);
    } else {
        release_threshold = baseline;
    }
    return release_threshold;
}

/* 测量触摸振荡耗时，短时间关中断避免 FreeRTOS tick 改变计数结果。 */
uint16_t captouch_read_channel(uint8_t channel)
{
    uint16_t cpu_count;
    uint16_t osc_count;
    uint16_t guard;
    uint16_t sr_state;
    uint8_t channel_bit;

    if (channel >= CAP_TOUCH_CHANNEL_COUNT) {
        return 0;
    }

    cpu_count = 0;
    osc_count = 0;
    guard = CAP_TOUCH_MEASURE_TIMEOUT;
    channel_bit = captouch_channel_bit(channel);

    CBCTL0 = (uint16_t)(CBIMEN | ((uint16_t)channel << 8));
    CAP_TOUCH_PORT_OUT &= (uint8_t)~CAP_TOUCH_ALL_BITS;
    CAP_TOUCH_PORT_DIR |= (uint8_t)(CAP_TOUCH_ALL_BITS & (uint8_t)~channel_bit);
    CBCTL3 = channel_bit;
    CBINT &= (uint16_t)~CBIFG;

    sr_state = __get_SR_register();
    __disable_interrupt();
    while (osc_count < CAP_TOUCH_OSC_CYCLES && guard > 0u) {
        if (CBCTL1 & CBOUT) {
            CAP_TOUCH_PORT_OUT |= CAP_TOUCH_ALL_BITS;
        } else {
            CAP_TOUCH_PORT_OUT &= (uint8_t)~CAP_TOUCH_ALL_BITS;
        }

        if (CBINT & CBIFG) {
            CBINT &= (uint16_t)~CBIFG;
            osc_count++;
        }

        cpu_count++;
        guard--;
    }
    if (sr_state & GIE) {
        __enable_interrupt();
    }

    CBCTL3 = CAP_TOUCH_ALL_BITS;
    CAP_TOUCH_PORT_DIR &= (uint8_t)~CAP_TOUCH_ALL_BITS;
    CAP_TOUCH_PORT_OUT &= (uint8_t)~CAP_TOUCH_ALL_BITS;

    if (guard == 0u) {
        return 0;
    }
    return cpu_count;
}

/* 初始化 Comparator B，并在默认未触摸条件下建立 Pad1/Pad2 基线。 */
void captouch_init(void)
{
    uint8_t channel;
    uint8_t sample;
    uint32_t total;
    uint16_t value;

    CAP_TOUCH_PORT_SEL &= (uint8_t)~CAP_TOUCH_ALL_BITS;
    CAP_TOUCH_PORT_REN &= (uint8_t)~CAP_TOUCH_ALL_BITS;
    CAP_TOUCH_PORT_OUT &= (uint8_t)~CAP_TOUCH_ALL_BITS;
    CAP_TOUCH_PORT_DIR &= (uint8_t)~CAP_TOUCH_ALL_BITS;

    CBCTL2 = CBREF14 | CBREF13 | CBREF02;
    CBCTL1 = CBON | CBF;
    CBCTL2 |= CBRS_1;
    CBCTL3 = CAP_TOUCH_ALL_BITS;
    CBINT &= (uint16_t)~(CBIFG | CBIIFG);

    for (channel = 0; channel < CAP_TOUCH_CHANNEL_COUNT; channel++) {
        total = 0;
        for (sample = 0; sample < CAP_TOUCH_BASELINE_SAMPLES; sample++) {
            value = captouch_read_channel(channel);
            total += value;
        }
        g_captouch_baseline[channel] = (uint16_t)(total / CAP_TOUCH_BASELINE_SAMPLES);
        g_captouch_press_samples[channel] = 0;
        g_captouch_release_samples[channel] = 0;
    }
    g_captouch_pressed_mask = 0;
}

/* 轮询两个触摸通道，返回已做按下/释放确认的 bit0=Pad1、bit1=Pad2。 */
uint8_t captouch_read_pressed_mask(void)
{
    uint8_t channel;
    uint8_t bit;
    uint16_t value;
    uint16_t press_threshold;
    uint16_t release_threshold;

    for (channel = 0; channel < CAP_TOUCH_CHANNEL_COUNT; channel++) {
        bit = captouch_channel_bit(channel);
        value = captouch_read_channel(channel);
        press_threshold = captouch_press_threshold(channel);
        release_threshold = captouch_release_threshold(channel);

        if (g_captouch_pressed_mask & bit) {
            if (value == 0u || value <= release_threshold) {
                if (g_captouch_release_samples[channel] < CAP_TOUCH_RELEASE_SAMPLES) {
                    g_captouch_release_samples[channel]++;
                }
            } else {
                g_captouch_release_samples[channel] = 0;
            }

            if (g_captouch_release_samples[channel] >= CAP_TOUCH_RELEASE_SAMPLES) {
                g_captouch_pressed_mask &= (uint8_t)~bit;
                g_captouch_press_samples[channel] = 0;
            }
        } else {
            if (value >= press_threshold) {
                if (g_captouch_press_samples[channel] < CAP_TOUCH_PRESS_SAMPLES) {
                    g_captouch_press_samples[channel]++;
                }
            } else {
                g_captouch_press_samples[channel] = 0;
                if (value != 0u && value < g_captouch_baseline[channel]) {
                    g_captouch_baseline[channel] = value;
                }
            }

            if (g_captouch_press_samples[channel] >= CAP_TOUCH_PRESS_SAMPLES) {
                g_captouch_pressed_mask |= bit;
                g_captouch_release_samples[channel] = 0;
            }
        }
    }
    return g_captouch_pressed_mask;
}
