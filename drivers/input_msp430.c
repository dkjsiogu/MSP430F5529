#include "input.h"

#include "board.h"
#include "captouch.h"
#include "platform_config.h"

typedef enum {
    INPUT_SOURCE_GPIO_PORT1 = 0u,                        /* P1 端口上的普通机械按键输入。 */
    INPUT_SOURCE_GPIO_PORT2,                             /* P2 端口上的普通机械按键输入。 */
    INPUT_SOURCE_TOUCH                                   /* Comparator B 电容触摸输入。 */
} InputSourceKind;

typedef struct {
    InputSourceKind kind;                                /* 输入源类型。 */
    uint8_t id;                                          /* GPIO 位或触摸通道编号。 */
    InputEvent event;                                    /* 该输入源触发的应用输入事件。 */
} InputBinding;

static const InputBinding g_input_bindings[] = {
    { INPUT_SOURCE_GPIO_PORT1, BUTTON_S1_BIT, INPUT_EVENT_PRIMARY },
    { INPUT_SOURCE_GPIO_PORT1, BUTTON_S2_BIT, INPUT_EVENT_SECONDARY },
    { INPUT_SOURCE_TOUCH, CAP_TOUCH_PAD1_CHANNEL, INPUT_EVENT_UP },
    { INPUT_SOURCE_TOUCH, CAP_TOUCH_PAD2_CHANNEL, INPUT_EVENT_DOWN },
    { INPUT_SOURCE_GPIO_PORT2, BUTTON_S3_BIT, INPUT_EVENT_BACK },
    { INPUT_SOURCE_GPIO_PORT2, BUTTON_S4_BIT, INPUT_EVENT_CONFIRM }
};

#define INPUT_BINDING_COUNT      ((uint8_t)(sizeof(g_input_bindings) / sizeof(g_input_bindings[0])))
#define INPUT_GPIO_PORT_COUNT    2u
#define INPUT_TOUCH_BIT(channel) ((uint8_t)(1u << (uint8_t)(channel)))

static volatile uint8_t g_input_irq_bits[INPUT_GPIO_PORT_COUNT] = {0, 0};
static uint8_t g_input_gpio_mask[INPUT_GPIO_PORT_COUNT] = {0, 0};
static uint8_t g_input_last_pressed[INPUT_GPIO_PORT_COUNT] = {0, 0};
static uint8_t g_touch_last_pressed = 0;
static uint8_t g_input_event_guard_seen = 0;
static uint16_t g_input_event_last_tick[INPUT_EVENT_COUNT];
static InputIsrWakeHook g_input_wake_hook = 0;

/* 从输入映射表预计算两个 GPIO 端口的机械按键位掩码。 */
static void input_build_gpio_masks(void)
{
    uint8_t i;

    g_input_gpio_mask[INPUT_SOURCE_GPIO_PORT1] = 0;
    g_input_gpio_mask[INPUT_SOURCE_GPIO_PORT2] = 0;
    for (i = 0; i < INPUT_BINDING_COUNT; i++) {
        if (g_input_bindings[i].kind == INPUT_SOURCE_GPIO_PORT1 ||
            g_input_bindings[i].kind == INPUT_SOURCE_GPIO_PORT2) {
            g_input_gpio_mask[g_input_bindings[i].kind] |= g_input_bindings[i].id;
        }
    }
}

/* 返回某个 GPIO 输入端口的机械按键位掩码。 */
static uint8_t input_gpio_port_mask(InputSourceKind kind)
{
    if (kind == INPUT_SOURCE_GPIO_PORT1 || kind == INPUT_SOURCE_GPIO_PORT2) {
        return g_input_gpio_mask[kind];
    }
    return 0;
}

/* 读取指定 GPIO 输入端口的稳定低电平按下状态。 */
static uint8_t input_read_gpio_pressed(InputSourceKind kind)
{
    uint8_t mask;

    mask = input_gpio_port_mask(kind);
    if (kind == INPUT_SOURCE_GPIO_PORT1) {
        return (uint8_t)((~BUTTON_PORT_IN) & mask);
    }
    if (kind == INPUT_SOURCE_GPIO_PORT2) {
        return (uint8_t)((~BUTTON2_PORT_IN) & mask);
    }
    return 0;
}

/* 按输入映射表初始化一个 GPIO 端口的上拉和下降沿中断。 */
static void input_init_gpio_port(InputSourceKind kind)
{
    uint8_t mask;

    mask = input_gpio_port_mask(kind);
    if (mask == 0) {
        return;
    }

    if (kind == INPUT_SOURCE_GPIO_PORT1) {
        BUTTON_PORT_SEL &= (uint8_t)~mask;
        BUTTON_PORT_DIR &= (uint8_t)~mask;
        BUTTON_PORT_OUT |= mask;
        BUTTON_PORT_REN |= mask;
        BUTTON_PORT_IES |= mask;
        BUTTON_PORT_IFG &= (uint8_t)~mask;
        BUTTON_PORT_IE |= mask;
    } else if (kind == INPUT_SOURCE_GPIO_PORT2) {
        BUTTON2_PORT_SEL &= (uint8_t)~mask;
        BUTTON2_PORT_DIR &= (uint8_t)~mask;
        BUTTON2_PORT_OUT |= mask;
        BUTTON2_PORT_REN |= mask;
        BUTTON2_PORT_IES |= mask;
        BUTTON2_PORT_IFG &= (uint8_t)~mask;
        BUTTON2_PORT_IE |= mask;
    }
}

/* 重新打开指定 GPIO 输入端口的中断，供去抖完成后恢复边沿触发。 */
static void input_rearm_gpio_port(InputSourceKind kind)
{
    uint8_t mask;

    mask = input_gpio_port_mask(kind);
    if (mask == 0) {
        return;
    }

    if (kind == INPUT_SOURCE_GPIO_PORT1) {
        BUTTON_PORT_IFG &= (uint8_t)~mask;
        BUTTON_PORT_IE |= mask;
    } else if (kind == INPUT_SOURCE_GPIO_PORT2) {
        BUTTON2_PORT_IFG &= (uint8_t)~mask;
        BUTTON2_PORT_IE |= mask;
    }
}

/* 根据每个输入事件独立的保护时间过滤重复边沿，减少释放抖动误触发。 */
static uint8_t input_event_allowed(uint8_t event)
{
    uint16_t now;
    uint8_t event_bit;

    if (event >= INPUT_EVENT_COUNT) {
        return 0;
    }
    now = board_tick10();
    event_bit = INPUT_EVENT_BIT(event);
    if ((g_input_event_guard_seen & event_bit) &&
        !board_tick10_elapsed(g_input_event_last_tick[event], BUTTON_REPEAT_GUARD_TICKS)) {
        return 0;
    }

    g_input_event_guard_seen |= event_bit;
    g_input_event_last_tick[event] = now;
    return 1;
}

/* 把一次输入源变化转换成稳定的新输入事件集合。 */
static uint8_t input_collect_events(uint8_t port1_pressed, uint8_t port2_pressed, uint8_t touch_pressed)
{
    uint8_t new_gpio_pressed[INPUT_GPIO_PORT_COUNT];
    uint8_t new_touch_pressed;
    uint8_t events;
    uint8_t event_bit;
    uint8_t i;

    new_gpio_pressed[INPUT_SOURCE_GPIO_PORT1] =
        (uint8_t)(port1_pressed & (uint8_t)~g_input_last_pressed[INPUT_SOURCE_GPIO_PORT1]);
    new_gpio_pressed[INPUT_SOURCE_GPIO_PORT2] =
        (uint8_t)(port2_pressed & (uint8_t)~g_input_last_pressed[INPUT_SOURCE_GPIO_PORT2]);
    new_touch_pressed = (uint8_t)(touch_pressed & (uint8_t)~g_touch_last_pressed);

    events = 0;
    for (i = 0; i < INPUT_BINDING_COUNT; i++) {
        if (g_input_bindings[i].kind == INPUT_SOURCE_GPIO_PORT1 &&
            (new_gpio_pressed[INPUT_SOURCE_GPIO_PORT1] & g_input_bindings[i].id) == 0) {
            continue;
        }
        if (g_input_bindings[i].kind == INPUT_SOURCE_GPIO_PORT2 &&
            (new_gpio_pressed[INPUT_SOURCE_GPIO_PORT2] & g_input_bindings[i].id) == 0) {
            continue;
        }
        if (g_input_bindings[i].kind == INPUT_SOURCE_TOUCH &&
            (new_touch_pressed & INPUT_TOUCH_BIT(g_input_bindings[i].id)) == 0) {
            continue;
        }

        event_bit = INPUT_EVENT_BIT(g_input_bindings[i].event);
        if ((events & event_bit) || input_event_allowed((uint8_t)g_input_bindings[i].event)) {
            events |= event_bit;
        }
    }
    return events;
}

void input_init(void)
{
    input_build_gpio_masks();
    input_init_gpio_port(INPUT_SOURCE_GPIO_PORT1);
    input_init_gpio_port(INPUT_SOURCE_GPIO_PORT2);
    captouch_init();
}

void input_set_wake_hook(InputIsrWakeHook hook)
{
    g_input_wake_hook = hook;
}

uint8_t input_take_events(void)
{
    uint8_t irq_bits[INPUT_GPIO_PORT_COUNT];
    uint8_t port1_pressed;
    uint8_t port2_pressed;
    uint8_t touch_pressed;
    uint8_t first_touch_pressed;
    uint8_t observed_touch_pressed;
    uint8_t events;

    port1_pressed = input_read_gpio_pressed(INPUT_SOURCE_GPIO_PORT1);
    port2_pressed = input_read_gpio_pressed(INPUT_SOURCE_GPIO_PORT2);
    touch_pressed = captouch_read_pressed_mask();
    first_touch_pressed = touch_pressed;
    irq_bits[INPUT_SOURCE_GPIO_PORT1] = g_input_irq_bits[INPUT_SOURCE_GPIO_PORT1];
    irq_bits[INPUT_SOURCE_GPIO_PORT2] = g_input_irq_bits[INPUT_SOURCE_GPIO_PORT2];
    if (irq_bits[INPUT_SOURCE_GPIO_PORT1] == 0 && irq_bits[INPUT_SOURCE_GPIO_PORT2] == 0 &&
        port1_pressed == g_input_last_pressed[INPUT_SOURCE_GPIO_PORT1] &&
        port2_pressed == g_input_last_pressed[INPUT_SOURCE_GPIO_PORT2] &&
        touch_pressed == g_touch_last_pressed) {
        return 0;
    }

    delay_ms(BUTTON_DEBOUNCE_MS);

    port1_pressed = input_read_gpio_pressed(INPUT_SOURCE_GPIO_PORT1);
    port2_pressed = input_read_gpio_pressed(INPUT_SOURCE_GPIO_PORT2);
    touch_pressed = captouch_read_pressed_mask();
    observed_touch_pressed = (uint8_t)(first_touch_pressed | touch_pressed);
    events = input_collect_events(port1_pressed, port2_pressed, observed_touch_pressed);

    g_input_irq_bits[INPUT_SOURCE_GPIO_PORT1] = 0;
    g_input_irq_bits[INPUT_SOURCE_GPIO_PORT2] = 0;
    g_input_last_pressed[INPUT_SOURCE_GPIO_PORT1] = port1_pressed;
    g_input_last_pressed[INPUT_SOURCE_GPIO_PORT2] = port2_pressed;
    g_touch_last_pressed = observed_touch_pressed;
    input_rearm_gpio_port(INPUT_SOURCE_GPIO_PORT1);
    input_rearm_gpio_port(INPUT_SOURCE_GPIO_PORT2);
    return events;
}

uint8_t input_pending(void)
{
    uint8_t port1_pressed;
    uint8_t port2_pressed;
    uint8_t touch_pressed;

    port1_pressed = input_read_gpio_pressed(INPUT_SOURCE_GPIO_PORT1);
    port2_pressed = input_read_gpio_pressed(INPUT_SOURCE_GPIO_PORT2);
    touch_pressed = captouch_read_pressed_mask();
    return (uint8_t)(g_input_irq_bits[INPUT_SOURCE_GPIO_PORT1] != 0 ||
                     g_input_irq_bits[INPUT_SOURCE_GPIO_PORT2] != 0 ||
                     (port1_pressed & (uint8_t)~g_input_last_pressed[INPUT_SOURCE_GPIO_PORT1]) != 0 ||
                     (port2_pressed & (uint8_t)~g_input_last_pressed[INPUT_SOURCE_GPIO_PORT2]) != 0 ||
                     (touch_pressed & (uint8_t)~g_touch_last_pressed) != 0);
}

#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
    uint8_t flags;
    uint8_t mask;

    mask = g_input_gpio_mask[INPUT_SOURCE_GPIO_PORT1];
    flags = (uint8_t)(BUTTON_PORT_IFG & mask);
    if (flags) {
        g_input_irq_bits[INPUT_SOURCE_GPIO_PORT1] |= flags;
        BUTTON_PORT_IE &= (uint8_t)~mask;
        BUTTON_PORT_IFG &= (uint8_t)~mask;
        if (g_input_wake_hook != 0) {
            g_input_wake_hook();
        }
        __bic_SR_register_on_exit(LPM0_bits);
    }
}

#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void)
{
    uint8_t flags;
    uint8_t mask;

    mask = g_input_gpio_mask[INPUT_SOURCE_GPIO_PORT2];
    flags = (uint8_t)(BUTTON2_PORT_IFG & mask);
    if (flags) {
        g_input_irq_bits[INPUT_SOURCE_GPIO_PORT2] |= flags;
        BUTTON2_PORT_IE &= (uint8_t)~mask;
        BUTTON2_PORT_IFG &= (uint8_t)~mask;
        if (g_input_wake_hook != 0) {
            g_input_wake_hook();
        }
        __bic_SR_register_on_exit(LPM0_bits);
    }
}
