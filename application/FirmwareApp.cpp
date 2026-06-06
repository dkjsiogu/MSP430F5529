#include "FirmwareApp.hpp"

#include "RuntimeState.hpp"
#include "../middleware/ButtonPort.hpp"
#include "../middleware/CBindings.hpp"

#include "FreeRTOS.h"
#include "task.h"

namespace application {

namespace {

typedef middleware::ButtonPort<RuntimeState> Buttons;

static RuntimeState g_runtime;
static StaticTask_t g_app_task_tcb;
static StackType_t g_app_task_stack[192];

void initialize()
{
    WDTCTL = WDTPW | WDTHOLD;

    g_runtime.reset();

    clock_init();
    app_state_init();
    gpio_init();
    Buttons::init();
    uart_init();
    sensors_init();
    flash_scan();
    app_resources_init();
    epd_init();
    sample_timer_init();
}

void loop_once()
{
    Buttons::task(g_runtime);

    serial_control_poll();

    if (sample_timer_take_due()) {
        collect_sample(&g_runtime.currentSample);
        g_runtime.markSampleCollected();
        board_toggle_heartbeat();
        if (sample_over_threshold(&g_runtime.currentSample)) {
            buzzer_alert_for(app_alarm_duration_seconds());
        } else {
            buzzer_set(0);
        }
        if (epd_auto_enabled()) {
            epd_show_current_auto(&g_runtime.currentSample);
        }
    }

    epd_render_task();

    if (!Buttons::pending() && !epd_render_pending()) {
        app_state_task();
        if (g_runtime.logPending) {
            flash_log_sample(&g_runtime.pendingLogSample);
            g_runtime.clearLogPending();
        }
    }
}

void app_task(void *)
{
    while (1) {
        loop_once();
        if (!Buttons::pending() && !epd_render_pending()) {
            vTaskDelay(1);
        }
    }
}

} // namespace

void start()
{
    initialize();

    (void)xTaskCreateStatic(app_task,
                            "app",
                            (uint32_t)(sizeof(g_app_task_stack) / sizeof(g_app_task_stack[0])),
                            0,
                            (UBaseType_t)(tskIDLE_PRIORITY + 1u),
                            g_app_task_stack,
                            &g_app_task_tcb);
    vTaskStartScheduler();

    while (1) {
    }
}

} // namespace application
