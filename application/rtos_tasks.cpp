#include "rtos_tasks.hpp"

#include "middleware/CBindings.hpp"

#include "FreeRTOS.h"
#include "task.h"

#define CONTROL_TASK_STACK_WORDS       96u
#define SAMPLE_TASK_STACK_WORDS        112u
#define DISPLAY_TASK_STACK_WORDS       192u
#define FLASH_TASK_STACK_WORDS         112u

#define CONTROL_TASK_PRIORITY          2u
#define SAMPLE_TASK_PRIORITY           2u
#define DISPLAY_TASK_PRIORITY          1u
#define FLASH_TASK_PRIORITY            1u

#define CONTROL_WAIT_MS                20u
#define SAMPLE_WAIT_MS                 20u
#define DISPLAY_WAIT_MS                50u
#define FLASH_WAIT_MS                  100u
#define RTOS_DELAY_YIELD_MS            10u

static StaticTask_t g_control_task_tcb;
static StaticTask_t g_sample_task_tcb;
static StaticTask_t g_display_task_tcb;
static StaticTask_t g_flash_task_tcb;

static StackType_t g_control_task_stack[CONTROL_TASK_STACK_WORDS];
static StackType_t g_sample_task_stack[SAMPLE_TASK_STACK_WORDS];
static StackType_t g_display_task_stack[DISPLAY_TASK_STACK_WORDS];
static StackType_t g_flash_task_stack[FLASH_TASK_STACK_WORDS];

static TaskHandle_t g_control_task_handle = 0;
static TaskHandle_t g_sample_task_handle = 0;
static TaskHandle_t g_display_task_handle = 0;
static TaskHandle_t g_flash_task_handle = 0;

static TempSample g_latest_sample;
static TempSample g_pending_log_sample;
static uint8_t g_has_latest_sample = 0;
static uint8_t g_log_pending = 0;
static uint8_t g_flash_erase_pending = 0;

static TickType_t rtos_ticks_from_ms(uint16_t ms)
{
    uint32_t ticks;

    ticks = ((uint32_t)ms * (uint32_t)configTICK_RATE_HZ + 999u) / 1000u;
    if (ticks == 0) {
        ticks = 1;
    }
    return (TickType_t)ticks;
}

static void rtos_delay_ms(uint16_t ms)
{
    if (ms == 0) {
        return;
    }
    if (ms < RTOS_DELAY_YIELD_MS) {
        while (ms--) {
            __delay_cycles(MCLK_HZ / 1000u);
        }
        return;
    }
    vTaskDelay(rtos_ticks_from_ms(ms));
}

static void notify_task(TaskHandle_t task)
{
    if (task != 0) {
        (void)xTaskNotifyGive(task);
    }
}

static void notify_task_from_isr(TaskHandle_t task)
{
    BaseType_t higher_priority_woken;

    if (task == 0) {
        return;
    }
    higher_priority_woken = pdFALSE;
    vTaskNotifyGiveFromISR(task, &higher_priority_woken);
    portYIELD_FROM_ISR(higher_priority_woken);
}

static void control_wake_from_isr(void)
{
    notify_task_from_isr(g_control_task_handle);
}

static void sample_due_from_isr(void)
{
    notify_task_from_isr(g_sample_task_handle);
}

static void latest_sample_store(const TempSample *sample)
{
    taskENTER_CRITICAL();
    g_latest_sample = *sample;
    g_has_latest_sample = 1;
    taskEXIT_CRITICAL();
}

static uint8_t latest_sample_load(TempSample *sample)
{
    uint8_t has_sample;

    taskENTER_CRITICAL();
    has_sample = g_has_latest_sample;
    if (has_sample) {
        *sample = g_latest_sample;
    }
    taskEXIT_CRITICAL();
    return has_sample;
}

static void flash_log_request(const TempSample *sample)
{
    taskENTER_CRITICAL();
    g_pending_log_sample = *sample;
    g_log_pending = 1;
    taskEXIT_CRITICAL();
    notify_task(g_flash_task_handle);
}

static uint8_t flash_log_take(TempSample *sample)
{
    uint8_t pending;

    taskENTER_CRITICAL();
    pending = g_log_pending;
    if (pending) {
        *sample = g_pending_log_sample;
        g_log_pending = 0;
    }
    taskEXIT_CRITICAL();
    return pending;
}

static void flash_erase_request(void)
{
    taskENTER_CRITICAL();
    g_flash_erase_pending = 1;
    taskEXIT_CRITICAL();
    notify_task(g_flash_task_handle);
}

static uint8_t flash_erase_take(void)
{
    uint8_t pending;

    taskENTER_CRITICAL();
    pending = g_flash_erase_pending;
    g_flash_erase_pending = 0;
    taskEXIT_CRITICAL();
    return pending;
}

static void control_task(void *argument)
{
    TempSample sample;
    uint8_t has_sample;

    (void)argument;
    for (;;) {
        has_sample = latest_sample_load(&sample);
        buttons_task(&sample, has_sample);
        serial_control_poll();

        if (epd_render_pending()) {
            notify_task(g_display_task_handle);
        }
        notify_task(g_sample_task_handle);

        (void)ulTaskNotifyTake(pdTRUE, rtos_ticks_from_ms(CONTROL_WAIT_MS));
    }
}

static void sample_task(void *argument)
{
    TempSample sample;

    (void)argument;
    for (;;) {
        if (sample_timer_take_due()) {
            collect_sample(&sample);
            latest_sample_store(&sample);
            board_toggle_heartbeat();

            if (sample_over_threshold(&sample)) {
                buzzer_alert_for(app_alarm_duration_seconds());
            } else {
                buzzer_set(0);
            }

            if (epd_auto_enabled()) {
                epd_show_current_auto(&sample);
                notify_task(g_display_task_handle);
            }

            flash_log_request(&sample);
        }

        (void)ulTaskNotifyTake(pdTRUE, rtos_ticks_from_ms(SAMPLE_WAIT_MS));
    }
}

static void display_task(void *argument)
{
    (void)argument;
    for (;;) {
        epd_render_task();
        (void)ulTaskNotifyTake(pdTRUE, rtos_ticks_from_ms(DISPLAY_WAIT_MS));
    }
}

static void flash_task(void *argument)
{
    TempSample sample;

    (void)argument;
    for (;;) {
        if (flash_erase_take()) {
            flash_erase_log();
            epd_show_history_page(0);
            notify_task(g_display_task_handle);
        }
        if (flash_log_take(&sample)) {
            flash_log_sample(&sample);
        }
        app_state_task();

        (void)ulTaskNotifyTake(pdTRUE, rtos_ticks_from_ms(FLASH_WAIT_MS));
    }
}

void app_tasks_create(void)
{
    g_control_task_handle = xTaskCreateStatic(control_task, "ctrl",
                                             CONTROL_TASK_STACK_WORDS, 0,
                                             CONTROL_TASK_PRIORITY,
                                             g_control_task_stack,
                                             &g_control_task_tcb);
    g_sample_task_handle = xTaskCreateStatic(sample_task, "sample",
                                            SAMPLE_TASK_STACK_WORDS, 0,
                                            SAMPLE_TASK_PRIORITY,
                                            g_sample_task_stack,
                                            &g_sample_task_tcb);
    g_display_task_handle = xTaskCreateStatic(display_task, "display",
                                             DISPLAY_TASK_STACK_WORDS, 0,
                                             DISPLAY_TASK_PRIORITY,
                                             g_display_task_stack,
                                             &g_display_task_tcb);
    g_flash_task_handle = xTaskCreateStatic(flash_task, "flash",
                                           FLASH_TASK_STACK_WORDS, 0,
                                           FLASH_TASK_PRIORITY,
                                           g_flash_task_stack,
                                           &g_flash_task_tcb);

    configASSERT(g_control_task_handle != 0);
    configASSERT(g_sample_task_handle != 0);
    configASSERT(g_display_task_handle != 0);
    configASSERT(g_flash_task_handle != 0);
}

void app_tasks_install_hooks(void)
{
    board_set_delay_hook(rtos_delay_ms);
    buttons_set_wake_hook(control_wake_from_isr);
    uart_set_rx_hook(control_wake_from_isr);
    sample_timer_set_due_hook(sample_due_from_isr);
    serial_control_set_flash_erase_handler(flash_erase_request);
}
