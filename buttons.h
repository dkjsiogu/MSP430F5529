/*
 * buttons.h
 * Pocket Kit S1-S4 按键接口：封装 GPIO 采集、去抖、事件分发
 * 和当前按键业务动作，主循环只需要调用统一任务入口。
 */
#ifndef BUTTONS_H
#define BUTTONS_H                                       /* 防止 buttons.h 被重复包含。 */

#include "app_types.h"

void buttons_init(void);                                   /* 初始化 S1-S4 按键 GPIO 和端口中断。 */
void buttons_task(const TempSample *last_sample, uint8_t has_sample); /* 轮询并处理一次 S1-S4 按键任务。 */
uint8_t buttons_pending(void);                            /* 判断是否已有未处理按键输入，供主循环优先调度。 */
void buttons_action_s1(const TempSample *last_sample, uint8_t has_sample); /* 执行 S1 当前功能：历史页返回、设置页上移或数值增加。 */
void buttons_action_s2(const TempSample *last_sample, uint8_t has_sample); /* 执行 S2 当前功能：主界面进历史页、设置页下移或数值降低。 */
void buttons_action_s3(const TempSample *last_sample, uint8_t has_sample); /* 执行 S3 当前功能：进入或退出设置界面。 */
void buttons_action_s4(const TempSample *last_sample, uint8_t has_sample); /* 执行 S4 当前功能：确认设置项或手动刷新主界面。 */

#endif
