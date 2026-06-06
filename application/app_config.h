/*
 * app_config.h
 * 应用层策略配置：采样周期、报警阈值、设置范围、历史页行数和数据格式常量。
 * 本文件不包含 MCU 寄存器、引脚、片内地址或具体外设控制时序，确保应用公共类型不绑定具体芯片。
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H                                      /* 防止 app_config.h 被重复包含。 */

#include <stdint.h>
#include "system_config.h"

#define SAMPLE_INTERVAL_SECONDS       3u                  /* 默认温度采样间隔，单位秒。 */
#define SAMPLE_INTERVAL_MIN_SECONDS   1u                  /* 设置界面允许的最小采样间隔，单位秒。 */
#define SAMPLE_INTERVAL_MAX_SECONDS   60u                 /* 设置界面允许的最大采样间隔，单位秒。 */
#define SAMPLE_INTERVAL_STEP_SECONDS  1u                  /* 每次输入调整采样间隔的步进，单位秒。 */

#define ALERT_THRESHOLD_T10           500                 /* 默认报警温度阈值，单位 0.1 摄氏度。 */
#define ALERT_THRESHOLD_MIN_T10       -400                /* 报警温度阈值下限，单位 0.1 摄氏度。 */
#define ALERT_THRESHOLD_MAX_T10       1250                /* 报警温度阈值上限，单位 0.1 摄氏度。 */
#define ALERT_THRESHOLD_STEP_T10      10                  /* 报警阈值输入步进，10 表示 1.0 摄氏度。 */

#define STORAGE_LIMIT_DEFAULT         200u                /* 默认历史记录保存条数。 */
#define STORAGE_LIMIT_MIN             10u                 /* 设置界面允许的最小历史记录条数。 */
#define STORAGE_LIMIT_MAX             990u                /* 设置界面允许的最大历史记录条数。 */
#define STORAGE_LIMIT_STEP            10u                 /* 历史记录条数输入步进。 */

#define ALARM_DURATION_SECONDS        3u                  /* 默认报警鸣叫时长，单位秒。 */
#define ALARM_DURATION_MIN_SECONDS    1u                  /* 设置界面允许的最短报警时长，单位秒。 */
#define ALARM_DURATION_MAX_SECONDS    30u                 /* 设置界面允许的最长报警时长，单位秒。 */
#define ALARM_DURATION_STEP_SECONDS   1u                  /* 报警时长输入步进，单位秒。 */

#define HOURGLASS_SECONDS             15u                 /* 默认沙漏动画周期，单位秒。 */
#define HOURGLASS_MIN_SECONDS         5u                  /* 设置界面允许的最短沙漏周期，单位秒。 */
#define HOURGLASS_MAX_SECONDS         60u                 /* 设置界面允许的最长沙漏周期，单位秒。 */
#define HOURGLASS_STEP_SECONDS        1u                  /* 沙漏周期输入步进，单位秒。 */
#define HOURGLASS_FLIP_TICKS          160u                /* 沙漏周期末尾翻转动画持续时间，单位 10ms。 */

#define HISTORY_PAGE_ROWS             5u                  /* 历史记录页面每屏显示的记录行数。 */

#define SETTINGS_MAGIC                0x51A7u             /* 设置记录有效性标识。 */
#define SETTINGS_SAVE_DELAY_TICKS     80u                 /* 设置修改后空闲约 0.8 秒再写入存储。 */
#define RECORD_MAGIC                  0xA55Au             /* 温度历史记录有效性标识。 */

#endif
