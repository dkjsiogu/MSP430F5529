/*
 * board.h
 * MSP430F5529 板级初始化和基础外设控制接口：时钟、GPIO、
 * 采样定时器、蜂鸣器 PWM 和心跳 LED 都由这个模块封装。
 */
#ifndef BOARD_H
#define BOARD_H

#include "app_types.h"

void delay_ms(uint16_t ms);                                /* 阻塞延时指定毫秒数。 */
void clock_init(void);                                     /* 初始化系统时钟和 SMCLK。 */
void gpio_init(void);                                      /* 初始化板上 GPIO 默认方向和输出状态。 */
void sample_timer_init(void);                              /* 初始化周期采样定时器。 */
uint8_t sample_timer_take_due(void);                       /* 取走一次采样到期标志，到期返回 1。 */
void sample_timer_force_due(void);                         /* 强制置位采样到期标志，让主循环立即采样。 */
uint16_t board_tick10(void);                               /* 获取 10ms 系统节拍，允许自然回绕。 */
uint8_t board_tick10_elapsed(uint16_t start_tick, uint16_t ticks); /* 判断从 start_tick 起是否已经过指定 10ms 节拍数。 */
void buzzer_set(uint8_t on);                               /* 开启或关闭蜂鸣器 PWM 报警。 */
void buzzer_beep(uint16_t ms);                             /* 让蜂鸣器短鸣指定毫秒数，并保持原报警状态。 */
void buzzer_alert_for(uint8_t seconds);                    /* 按设置的秒数启动一次非阻塞报警鸣叫。 */
void board_toggle_heartbeat(void);                         /* 翻转心跳 LED，用于观察主循环采样节拍。 */

#endif
