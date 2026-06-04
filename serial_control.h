/*
 * serial_control.h
 * 串口接收侧控制接口：消费单字符命令并映射成本地动作，
 * 不向串口发送任何响应。
 */
#ifndef SERIAL_CONTROL_H
#define SERIAL_CONTROL_H

void serial_control_poll(void);                            /* 读取一个待处理串口字符并执行对应本地控制。 */

#endif
