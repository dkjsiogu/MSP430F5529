/*
 * system_config.h
 * 系统级公共时基配置：供板级定时器、RTOS 任务和显示节奏共同使用。
 */
#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H                                  /* 防止 system_config.h 被重复包含。 */

#define BOARD_TICK_HZ                 100u               /* 系统节拍频率，100Hz 表示 10ms 一次。 */
#define BOARD_TICKS_PER_SECOND        100u               /* 1 秒对应的 10ms 节拍数量。 */

#endif
