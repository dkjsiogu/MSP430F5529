#include "app_state.h"
#include "app_resources.h"
#include "board.h"
#include "buttons.h"
#include "epaper.h"
#include "flash_log.h"
#include "serial_control.h"
#include "sensors.h"
#include "uart.h"

int main(void)
{
    TempSample sample;
    TempSample pending_log_sample;
    uint8_t log_pending;
    uint8_t has_sample;

    WDTCTL = WDTPW | WDTHOLD;
    log_pending = 0;
    has_sample = 0;

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

    __enable_interrupt();

    while (1) {
        buttons_task(&sample, has_sample);

        serial_control_poll();

        if (sample_timer_take_due()) {
            collect_sample(&sample);
            has_sample = 1;
            board_toggle_heartbeat();
            if (sample_over_threshold(&sample)) {
                buzzer_alert_for(app_alarm_duration_seconds());
            } else {
                buzzer_set(0);
            }
            if (epd_auto_enabled()) {
                epd_show_current_auto(&sample);
            }
            pending_log_sample = sample;
            log_pending = 1;
        }

        buttons_task(&sample, has_sample);

        epd_render_task();

        buttons_task(&sample, has_sample);

        if (!buttons_pending() && !epd_render_pending()) {
            app_state_task();
            if (log_pending) {
                flash_log_sample(&pending_log_sample);
                log_pending = 0;
            }
        }

        if (buttons_pending()) {
            continue;
        }

        if (epd_render_pending()) {
            continue;
        }

        __bis_SR_register(LPM0_bits | GIE);
        __no_operation();
    }
}
