/*
 * uart.h
 * UART1 通信接口：接收路径用于单字符命令，发送路径用于调试和温度上报。
 */
#ifndef UART_H
#define UART_H                                          /* 防止 uart.h 被重复包含。 */

#include "app_types.h"

typedef void (*UartIsrWakeHook)(void);                     /* ISR 唤醒钩子：回调内部必须使用 ISR-safe API。 */

void uart_init(void);                                      /* 初始化 UART1 收发和接收中断。 */
void uart_set_rx_hook(UartIsrWakeHook hook);               /* 设置 UART RX 中断唤醒钩子；传入 0 禁用。 */
uint8_t uart_take_rx(void);                                /* 取走最近收到的命令字符，没有新字符返回 0。 */
void uart_write_char(uint8_t c);                           /* 发送一个字符，TX 短时忙时等待。 */
void uart_write_str(const char *s);                        /* 发送以 0 结尾的 ASCII 字符串。 */

#endif
