#include "middleware/CBindings.hpp"
#include "application/rtos_tasks.hpp"
#include "middleware/freertos/StaticThread.hpp"

#include "FreeRTOS.h"
#include "task.h"

/* main.cpp 持有应用线程对象，让启动入口直接展示线程数量、栈大小和优先级。 */
static freertos::StaticThread<application::tasks::control_stack_words,
                              application::tasks::control_priority> control_thread;
static freertos::StaticThread<application::tasks::sample_stack_words,
                              application::tasks::sample_priority> sample_thread;
static freertos::StaticThread<application::tasks::display_stack_words,
                              application::tasks::display_priority> display_thread;
static freertos::StaticThread<application::tasks::flash_stack_words,
                              application::tasks::flash_priority> flash_thread;

enum ApplicationThreadId {
    control_thread_id,
    sample_thread_id,
    display_thread_id,
    flash_thread_id
};

/* 将编译期线程编号映射到实际线程对象，供通知模板生成 C 回调函数。 */
template<ApplicationThreadId ThreadId>
static freertos::Thread &application_thread()
{
    switch (ThreadId) {
    case control_thread_id:
        return control_thread;
    case sample_thread_id:
        return sample_thread;
    case display_thread_id:
        return display_thread;
    case flash_thread_id:
        return flash_thread;
    }
    return control_thread;
}

/* 普通任务上下文使用的线程通知回调模板。 */
template<ApplicationThreadId ThreadId>
static void notify_thread()
{
    application_thread<ThreadId>().notify();
}

/* ISR 上下文使用的线程通知回调模板。 */
template<ApplicationThreadId ThreadId>
static void notify_thread_from_isr()
{
    application_thread<ThreadId>().notify_from_isr();
}

int main()
{
    application::tasks::Notifications task_notifications = {
        notify_thread<sample_thread_id>,
        notify_thread<display_thread_id>,
        notify_thread<flash_thread_id>
    };

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

    application::tasks::bind_notifications(task_notifications);
    board_set_delay_hook(application::tasks::delay_ms);
    buttons_set_wake_hook(notify_thread_from_isr<control_thread_id>);
    uart_set_rx_hook(notify_thread_from_isr<control_thread_id>);
    sample_timer_set_due_hook(notify_thread_from_isr<sample_thread_id>);
    serial_control_set_flash_erase_handler(application::tasks::request_flash_erase);

    vTaskStartScheduler();

    freertos::halt();
}
