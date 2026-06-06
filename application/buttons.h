/*
 * buttons.h
 * Pocket Kit 物理输入接口：封装 GPIO 采集、去抖、语义动作分发
 * 和当前按键业务状态，主循环只需要调用统一任务入口。
 */
#ifndef BUTTONS_H
#define BUTTONS_H                                       /* 防止 buttons.h 被重复包含。 */

#include "app_types.h"

typedef void (*ButtonsIsrWakeHook)(void);                  /* ISR 唤醒钩子：回调内部必须使用 ISR-safe API。 */

void buttons_init(void);                                   /* 初始化 S1-S4 GPIO 输入和 Pad1/Pad2 电容触摸输入。 */
void buttons_set_wake_hook(ButtonsIsrWakeHook hook);       /* 设置按键中断唤醒钩子；传入 0 禁用。 */
void buttons_task(const TempSample *last_sample, uint8_t has_sample); /* 轮询并处理一次物理输入任务。 */
uint8_t buttons_pending(void);                            /* 判断是否已有未处理按键输入，供主循环优先调度。 */

#endif
