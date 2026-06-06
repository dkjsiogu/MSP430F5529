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

struct ControlThreadSlot {
    static freertos::Thread &thread()
    {
        return control_thread;
    }
};

struct SampleThreadSlot {
    static freertos::Thread &thread()
    {
        return sample_thread;
    }
};

struct DisplayThreadSlot {
    static freertos::Thread &thread()
    {
        return display_thread;
    }
};

struct FlashThreadSlot {
    static freertos::Thread &thread()
    {
        return flash_thread;
    }
};

/* 线程通知模板：Slot 暴露具体线程对象，模板生成可传给 C 模块的无参回调。 */
template<class Slot>
static void notify_thread()
{
    Slot::thread().notify();
}

/* ISR 线程通知模板：中断里使用 ISR-safe 的 FreeRTOS 通知接口。 */
template<class Slot>
static void notify_thread_from_isr()
{
    Slot::thread().notify_from_isr();
}

int main()
{
    application::tasks::Notifications task_notifications = {
        notify_thread<SampleThreadSlot>,
        notify_thread<DisplayThreadSlot>,
        notify_thread<FlashThreadSlot>
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
    buttons_set_wake_hook(notify_thread_from_isr<ControlThreadSlot>);
    uart_set_rx_hook(notify_thread_from_isr<ControlThreadSlot>);
    sample_timer_set_due_hook(notify_thread_from_isr<SampleThreadSlot>);
    serial_control_set_flash_erase_handler(application::tasks::request_flash_erase);

    vTaskStartScheduler();

    freertos::halt();
}
