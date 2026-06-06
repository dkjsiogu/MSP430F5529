/*
 * board.h
 * MSP430F5529 板级初始化和基础外设控制接口：时钟、GPIO、
 * 采样定时器、蜂鸣器 PWM 和心跳 LED 都由这个模块封装。
 */
#ifndef BOARD_H
#define BOARD_H                                         /* 防止 board.h 被重复包含。 */

#include <stdint.h>

#define SAMPLE_TIMER_DUE_PERIODIC      0x01u             /* 周期 1 秒节拍到期，是否采样由应用层按当前间隔判断。 */
#define SAMPLE_TIMER_DUE_FORCED        0x02u             /* 应用主动要求立即采样。 */

typedef void (*BoardDelayHook)(uint16_t ms);               /* 可选延时钩子：RTOS 启动后用于把长阻塞延时交还给调度器。 */
typedef void (*BoardIsrWakeHook)(void);                    /* ISR 唤醒钩子：由定时器中断调用，回调内部必须使用 ISR-safe API。 */

void delay_ms(uint16_t ms);                                /* 阻塞延时指定毫秒数。 */
void board_busy_delay_ms(uint16_t ms);                     /* 使用板级忙等执行短延时，供调度器内极短硬件时序使用。 */
void board_set_delay_hook(BoardDelayHook hook);            /* 设置延时钩子；传入 0 恢复默认忙等延时。 */
void clock_init(void);                                     /* 初始化系统时钟和 SMCLK。 */
void gpio_init(void);                                      /* 初始化板上 GPIO 默认方向和输出状态。 */
void sample_timer_init(void);                              /* 初始化周期采样定时器。 */
void sample_timer_set_due_hook(BoardIsrWakeHook hook);     /* 设置采样到期 ISR 唤醒钩子；传入 0 禁用。 */
uint8_t sample_timer_take_due(void);                       /* 取走采样定时器事件标志，返回 SAMPLE_TIMER_DUE_* 位集合。 */
void sample_timer_force_due(void);                         /* 强制置位采样到期标志，让主循环立即采样。 */
uint16_t board_tick10(void);                               /* 获取 10ms 系统节拍，允许自然回绕。 */
uint8_t board_tick10_elapsed(uint16_t start_tick, uint16_t ticks); /* 判断从 start_tick 起是否已经过指定 10ms 节拍数。 */
void buzzer_set(uint8_t on);                               /* 开启或关闭蜂鸣器 PWM 报警。 */
void buzzer_beep(uint16_t ms);                             /* 让蜂鸣器短鸣指定毫秒数，并保持原报警状态。 */
void buzzer_alert_for(uint8_t seconds);                    /* 按设置的秒数启动一次非阻塞报警鸣叫。 */
void board_toggle_heartbeat(void);                         /* 翻转心跳 LED，用于观察主循环采样节拍。 */

#endif
