/*
 * app_state.h
 * 应用运行状态接口：管理采样间隔、报警阈值、存储条数、报警时长和设置持久化，
 * 和温度有效性判断，给主循环、蜂鸣器和显示模块提供统一入口。
 */
#ifndef APP_STATE_H
#define APP_STATE_H                                     /* 防止 app_state.h 被重复包含。 */

#include "app_types.h"

void app_state_init(void);                                 /* 启动时从 Info Flash 加载持久化应用设置。 */
void app_state_task(void);                                 /* 在显示刷新后补写待保存的应用状态。 */
void app_save_settings(void);                              /* 将当前应用设置追加保存到 Info Flash。 */
void app_flush_settings(void);                             /* 如有待保存设置，立即写入 Info Flash。 */
uint8_t app_sample_interval(void);                         /* 获取当前定时采样间隔，单位秒。 */
uint8_t app_alarm_duration_seconds(void);                  /* 获取当前报警鸣叫时长，单位秒。 */
uint16_t app_storage_limit(void);                          /* 获取当前 Flash 历史记录目标存储条数。 */
int16_t app_threshold_t10(void);                           /* 获取当前报警阈值，单位 0.1 摄氏度。 */
uint8_t app_adjust_sample_interval(int8_t delta_seconds);  /* 调整并持久化采样间隔，返回新间隔秒数。 */
int16_t app_adjust_threshold_t10(int16_t delta_t10);        /* 调整并持久化报警阈值，返回新阈值。 */
uint16_t app_adjust_storage_limit(int16_t delta_records);   /* 调整并持久化历史记录存储条数，返回新条数。 */
uint8_t app_adjust_alarm_duration(int8_t delta_seconds);    /* 调整并持久化报警鸣叫时长，返回新秒数。 */
uint8_t temp_is_valid(int16_t t10);                         /* 判断一个 0.1 摄氏度温度值是否有效。 */
uint8_t sample_over_threshold(const TempSample *s);         /* 判断样本中是否有任一路温度超过报警阈值。 */

#endif
