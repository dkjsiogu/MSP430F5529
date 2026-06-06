/*
 * settings_store.h
 * 应用设置持久化后端接口：隐藏具体存储介质、记录布局、擦写流程和校验细节。
 */
#ifndef SETTINGS_STORE_H
#define SETTINGS_STORE_H                                /* 防止 settings_store.h 被重复包含。 */

#include "app_types.h"

typedef struct {
    uint8_t sample_interval;                             /* 采样间隔，单位秒。 */
    uint8_t alarm_duration;                              /* 报警鸣叫时长，单位秒。 */
    uint8_t hourglass_seconds;                           /* 沙漏动画周期，单位秒。 */
    int16_t threshold_t10;                               /* 报警阈值，单位 0.1 摄氏度。 */
    uint16_t storage_limit;                              /* 历史记录目标保存条数。 */
} SettingsStoreValue;

uint8_t settings_store_load(SettingsStoreValue *value);  /* 加载最近一条有效设置，成功返回 1。 */
uint8_t settings_store_save(const SettingsStoreValue *value); /* 追加保存一条设置，成功返回 1。 */

#endif
