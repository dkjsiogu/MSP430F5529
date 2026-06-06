#include "middleware/CBindings.hpp"
#include "application/rtos_tasks.hpp"

#include "FreeRTOS.h"
#include "task.h"

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;

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

    app_tasks_create();
    app_tasks_install_hooks();
    vTaskStartScheduler();

    while (1) {
        ;
    }
}
