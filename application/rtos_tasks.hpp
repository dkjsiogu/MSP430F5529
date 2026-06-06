/*
 * 应用层 FreeRTOS 任务入口和任务间通知接口。
 * 线程对象由 main.cpp 持有，这里只暴露任务函数、栈配置和应用通知端口。
 */
#ifndef APPLICATION_RTOS_TASKS_HPP
#define APPLICATION_RTOS_TASKS_HPP

#include <stdint.h>

namespace application {
namespace tasks {

/* 控制线程栈大小，处理按键、串口和页面状态调度。 */
static const unsigned control_stack_words = 96u;
/* 采样线程栈大小，处理温度采集、报警和记录请求。 */
static const unsigned sample_stack_words = 112u;
/* 显示线程栈大小，处理墨水屏渲染任务。 */
static const unsigned display_stack_words = 192u;
/* Flash 线程栈大小，处理历史记录和设置持久化。 */
static const unsigned flash_stack_words = 112u;

/* 控制线程优先级，交互响应优先于显示和 Flash。 */
static const unsigned control_priority = 2u;
/* 采样线程优先级，保证定时采样和报警及时处理。 */
static const unsigned sample_priority = 2u;
/* 显示线程优先级，墨水屏刷新耗时较长，放在较低优先级。 */
static const unsigned display_priority = 1u;
/* Flash 线程优先级，Flash 写入不阻塞交互和采样。 */
static const unsigned flash_priority = 1u;

/* 应用任务之间使用的无参通知回调，由 main.cpp 绑定到具体线程对象。 */
typedef void (*NotifyHook)();

/* 应用任务可唤醒的目标线程集合。 */
struct Notifications {
    /* 唤醒采样线程。 */
    NotifyHook sample;
    /* 唤醒显示线程。 */
    NotifyHook display;
    /* 唤醒 Flash 线程。 */
    NotifyHook flash;
};

/* 绑定应用任务内部使用的线程通知端口。 */
void bind_notifications(const Notifications &notifications);
/* RTOS 启动后的毫秒级延时接口，替换旧的忙等 delay hook。 */
void delay_ms(uint16_t ms);
/* 请求 Flash 线程执行全擦并刷新历史页。 */
void request_flash_erase();

/* 控制任务入口，处理按键状态机、串口命令和页面刷新调度。 */
void control(void *argument);
/* 采样任务入口，处理定时采样、报警、显示请求和日志请求。 */
void sample(void *argument);
/* 显示任务入口，处理墨水屏渲染队列。 */
void display(void *argument);
/* Flash 任务入口，处理历史记录写入、擦除和应用设置保存。 */
void flash(void *argument);

}
}

#endif
