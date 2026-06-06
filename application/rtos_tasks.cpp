#include "rtos_tasks.hpp"

#include "middleware/CBindings.hpp"

#include "FreeRTOS.h"
#include "task.h"

#define CONTROL_WAIT_MS                20u
#define SAMPLE_WAIT_MS                 20u
#define DISPLAY_WAIT_MS                50u
#define FLASH_WAIT_MS                  100u
#define RTOS_DELAY_YIELD_MS            10u

static TempSample g_latest_sample;
static TempSample g_pending_log_sample;
static uint8_t g_has_latest_sample = 0;
static uint8_t g_log_pending = 0;
static uint8_t g_flash_erase_pending = 0;
static uint8_t g_sample_elapsed_seconds = 0;
static application::tasks::Notifications g_notifications = {0, 0, 0};

/* 把毫秒延时换算成 FreeRTOS tick，向上取整避免延时为 0。 */
static TickType_t rtos_ticks_from_ms(uint16_t ms)
{
    uint32_t ticks;

    ticks = ((uint32_t)ms * (uint32_t)configTICK_RATE_HZ + 999u) / 1000u;
    if (ticks == 0) {
        ticks = 1;
    }
    return (TickType_t)ticks;
}

/* 调用已绑定的任务通知回调，允许通知端口未绑定时安全跳过。 */
static void notify(application::tasks::NotifyHook hook)
{
    if (hook != 0) {
        hook();
    }
}

namespace application {
namespace tasks {

void delay_ms(uint16_t ms)
{
    if (ms == 0) {
        return;
    }
    if (ms < RTOS_DELAY_YIELD_MS) {
        board_busy_delay_ms(ms);
        return;
    }
    vTaskDelay(rtos_ticks_from_ms(ms));
}

void bind_notifications(const Notifications &notifications)
{
    g_notifications = notifications;
}

}
}

/* 保存最新采样值，供控制任务读取并参与按键页面状态机。 */
static void latest_sample_store(const TempSample *sample)
{
    taskENTER_CRITICAL();
    g_latest_sample = *sample;
    g_has_latest_sample = 1;
    taskEXIT_CRITICAL();
}

/* 读取最新采样值，返回当前是否已经采到有效样本。 */
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

/* 把采样任务生成的样本提交给 Flash 任务异步写入。 */
static void flash_log_request(const TempSample *sample)
{
    taskENTER_CRITICAL();
    g_pending_log_sample = *sample;
    g_log_pending = 1;
    taskEXIT_CRITICAL();
    notify(g_notifications.flash);
}

/* Flash 任务取走一条待写入样本，取走后清除 pending 标志。 */
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

/* Flash 任务取走一次擦除请求，取走后清除 pending 标志。 */
static uint8_t flash_erase_take(void)
{
    uint8_t pending;

    taskENTER_CRITICAL();
    pending = g_flash_erase_pending;
    g_flash_erase_pending = 0;
    taskEXIT_CRITICAL();
    return pending;
}

/* 设置 Flash 擦除请求，并唤醒 Flash 任务执行耗时操作。 */
static void flash_erase_request(void)
{
    taskENTER_CRITICAL();
    g_flash_erase_pending = 1;
    taskEXIT_CRITICAL();
    notify(g_notifications.flash);
}

/* 将板级周期/强制事件转换成应用采样决策，采样间隔只在应用任务层读取。 */
static uint8_t sample_due_should_collect(uint8_t due_flags)
{
    if (due_flags & SAMPLE_TIMER_DUE_FORCED) {
        g_sample_elapsed_seconds = 0;
        return 1;
    }

    if (due_flags & SAMPLE_TIMER_DUE_PERIODIC) {
        if (g_sample_elapsed_seconds < 250u) {
            g_sample_elapsed_seconds++;
        }
        if (g_sample_elapsed_seconds >= app_sample_interval()) {
            g_sample_elapsed_seconds = 0;
            return 1;
        }
    }

    return 0;
}

namespace application {
namespace tasks {

void request_flash_erase()
{
    flash_erase_request();
}

void control(void *argument)
{
    TempSample sample;
    uint8_t has_sample;

    (void)argument;
    for (;;) {
        has_sample = latest_sample_load(&sample);
        interaction_task(&sample, has_sample);
        serial_control_poll();

        if (epd_render_pending()) {
            notify(g_notifications.display);
        }
        notify(g_notifications.sample);

        (void)ulTaskNotifyTake(pdTRUE, rtos_ticks_from_ms(CONTROL_WAIT_MS));
    }
}

void sample(void *argument)
{
    TempSample sample;
    uint8_t due_flags;

    (void)argument;
    for (;;) {
        due_flags = sample_timer_take_due();
        if (sample_due_should_collect(due_flags)) {
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
                notify(g_notifications.display);
            }

            flash_log_request(&sample);
        }

        (void)ulTaskNotifyTake(pdTRUE, rtos_ticks_from_ms(SAMPLE_WAIT_MS));
    }
}

void display(void *argument)
{
    (void)argument;
    for (;;) {
        epd_render_task();
        (void)ulTaskNotifyTake(pdTRUE, rtos_ticks_from_ms(DISPLAY_WAIT_MS));
    }
}

void flash(void *argument)
{
    TempSample sample;

    (void)argument;
    for (;;) {
        if (flash_erase_take()) {
            flash_erase_log();
            epd_show_history_page(0);
            notify(g_notifications.display);
        }
        if (flash_log_take(&sample)) {
            flash_log_sample(&sample);
        }
        app_state_task();

        (void)ulTaskNotifyTake(pdTRUE, rtos_ticks_from_ms(FLASH_WAIT_MS));
    }
}

}
}
