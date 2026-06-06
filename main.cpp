#include "middleware/CBindings.hpp"

#include "FreeRTOS.h"
#include "task.h"

#define APP_TASK_STACK_WORDS 192u

static TempSample g_sample;
static TempSample g_pending_log_sample;
static uint8_t g_log_pending;
static uint8_t g_has_sample;
static StaticTask_t g_app_task_tcb;
static StackType_t g_app_task_stack[APP_TASK_STACK_WORDS];

static void app_task(void *)
{
    while (1) {
        buttons_task(&g_sample, g_has_sample);

        serial_control_poll();

        if (sample_timer_take_due()) {
            collect_sample(&g_sample);
            g_has_sample = 1;
            board_toggle_heartbeat();
            if (sample_over_threshold(&g_sample)) {
                buzzer_alert_for(app_alarm_duration_seconds());
            } else {
                buzzer_set(0);
            }
            if (epd_auto_enabled()) {
                epd_show_current_auto(&g_sample);
            }
            g_pending_log_sample = g_sample;
            g_log_pending = 1;
        }

        epd_render_task();

        if (!buttons_pending() && !epd_render_pending()) {
            app_state_task();
            if (g_log_pending) {
                flash_log_sample(&g_pending_log_sample);
                g_log_pending = 0;
            }
        }

        if (!buttons_pending() && !epd_render_pending()) {
            vTaskDelay(1);
        }
    }
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;
    g_log_pending = 0;
    g_has_sample = 0;

    clock_init();
    app_state_init();
    gpio_init();
    buttons_init();
    uart_init();
    sensors_init();
    flash_scan();
    app_resources_init();
    epd_init();
    sample_timer_init();

    (void)xTaskCreateStatic(app_task,
                            "app",
                            APP_TASK_STACK_WORDS,
                            0,
                            (UBaseType_t)(tskIDLE_PRIORITY + 1u),
                            g_app_task_stack,
                            &g_app_task_tcb);
    vTaskStartScheduler();

    while (1) {
    }
}
