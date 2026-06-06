/*
 * FreeRTOS 静态线程 C++ 适配层。
 * 只封装静态任务创建和任务通知，不使用堆、异常、RTTI 或 STL。
 */
#ifndef MIDDLEWARE_FREERTOS_STATIC_THREAD_HPP
#define MIDDLEWARE_FREERTOS_STATIC_THREAD_HPP

#include "FreeRTOS.h"
#include "task.h"

namespace freertos {

/* FreeRTOS 任务句柄的轻量 C++ 包装，提供普通上下文和 ISR 上下文通知接口。 */
class Thread {
public:
    /* 从普通任务上下文唤醒该线程。 */
    void notify() const
    {
        if (handle_ != 0) {
            (void)xTaskNotifyGive(handle_);
        }
    }

    /* 从中断上下文唤醒该线程，并在需要时请求一次调度切换。 */
    void notify_from_isr() const
    {
        BaseType_t higher_priority_woken;

        if (handle_ == 0) {
            return;
        }
        higher_priority_woken = pdFALSE;
        vTaskNotifyGiveFromISR(handle_, &higher_priority_woken);
        portYIELD_FROM_ISR(higher_priority_woken);
    }

    /* 返回底层 FreeRTOS 任务句柄，供必须直接调用内核 API 的代码使用。 */
    TaskHandle_t native_handle() const
    {
        return handle_;
    }

protected:
    /* 仅供 StaticThread 在创建成功后写入底层任务句柄。 */
    void set_native_handle(TaskHandle_t handle)
    {
        handle_ = handle;
    }

private:
    TaskHandle_t handle_;
};

/* 编译期固定栈大小和优先级的静态线程，不走 FreeRTOS 动态内存分配。 */
template<unsigned StackWords, unsigned Priority>
class StaticThread : public Thread {
public:
    /* 用给定入口函数创建静态 FreeRTOS 任务。 */
    void create(const char *name, TaskFunction_t entry, void *argument = 0)
    {
        set_native_handle(xTaskCreateStatic(entry, name,
                                            static_cast<configSTACK_DEPTH_TYPE>(StackWords),
                                            argument,
                                            static_cast<UBaseType_t>(Priority),
                                            stack_,
                                            &control_block_));

        configASSERT(native_handle() != 0);
    }

private:
    StaticTask_t control_block_;
    StackType_t stack_[StackWords];
};

/* 调度器启动失败后的停机兜底，正常运行时不会返回到这里。 */
inline void halt()
{
    for (;;) {
        __no_operation();
    }
}

}

#endif
