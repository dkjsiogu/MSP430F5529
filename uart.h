/*
 * uart.h
 * UART1 接收接口：应用层只读取一个待处理命令字符，
 * 本模块不再负责样本、状态或调试文本输出。
 */
#ifndef UART_H
#define UART_H                                          /* 防止 uart.h 被重复包含。 */

#include "app_types.h"

typedef void (*UartIsrWakeHook)(void);                     /* ISR 唤醒钩子：回调内部必须使用 ISR-safe API。 */

void uart_init(void);                                      /* 初始化 UART1 接收中断。 */
void uart_set_rx_hook(UartIsrWakeHook hook);               /* 设置 UART RX 中断唤醒钩子；传入 0 禁用。 */
uint8_t uart_take_rx(void);                                /* 取走最近收到的命令字符，没有新字符返回 0。 */

#endif
