/*
 * uart.h
 * UART1 接收接口：应用层只读取一个待处理命令字符，
 * 本模块不再负责样本、状态或调试文本输出。
 */
#ifndef UART_H
#define UART_H                                          /* 防止 uart.h 被重复包含。 */

#include "app_types.h"

void uart_init(void);                                      /* 初始化 UART1 接收中断。 */
uint8_t uart_take_rx(void);                                /* 取走最近收到的命令字符，没有新字符返回 0。 */

#endif
