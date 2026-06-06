/*
 * interaction.h
 * 应用层输入业务状态机：消费 input.h 提供的输入事件，处理页面切换、
 * 设置调整、阅读翻页和手动刷新等最高层交互逻辑。
 */
#ifndef INTERACTION_H
#define INTERACTION_H                                   /* 防止 interaction.h 被重复包含。 */

#include "app_types.h"

void interaction_task(const TempSample *last_sample, uint8_t has_sample); /* 消费输入事件并推进一次页面交互状态机。 */
uint8_t interaction_pending(void);                         /* 判断是否已有未处理输入事件，供控制任务优先调度。 */

#endif
