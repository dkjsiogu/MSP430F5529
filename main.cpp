#include "middleware/CBindings.hpp"
#include "application/rtos_tasks.hpp"
#include "middleware/freertos/StaticThread.hpp"

#include "FreeRTOS.h"
#include "task.h"

/* main.cpp 持有应用线程对象，让启动入口直接展示线程数量、栈大小和优先级。 */
enum ApplicationThreadId {
    control_thread_id,
    sample_thread_id,
    display_thread_id,
    flash_thread_id
};

static freertos::StaticThread<application::tasks::control_stack_words,
                              application::tasks::control_priority,
                              control_thread_id> control_thread;
static freertos::StaticThread<application::tasks::sample_stack_words,
                              application::tasks::sample_priority,
                              sample_thread_id> sample_thread;
static freertos::StaticThread<application::tasks::display_stack_words,
                              application::tasks::display_priority,
                              display_thread_id> display_thread;
static freertos::StaticThread<application::tasks::flash_stack_words,
                              application::tasks::flash_priority,
                              flash_thread_id> flash_thread;

int main()
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

    control_thread.create("ctrl", application::tasks::control);
    sample_thread.create("sample", application::tasks::sample);
    display_thread.create("display", application::tasks::display);
    flash_thread.create("flash", application::tasks::flash);

    application::tasks::Notifications task_notifications = {
        sample_thread.notify_hook(),
        display_thread.notify_hook(),
        flash_thread.notify_hook()
    };

    application::tasks::bind_notifications(task_notifications);
    board_set_delay_hook(application::tasks::delay_ms);
    buttons_set_wake_hook(control_thread.notify_from_isr_hook());
    uart_set_rx_hook(control_thread.notify_from_isr_hook());
    sample_timer_set_due_hook(sample_thread.notify_from_isr_hook());
    serial_control_set_flash_erase_handler(application::tasks::request_flash_erase);

    vTaskStartScheduler();

    freertos::halt();
}
