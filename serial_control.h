/*
 * serial_control.h
 * 串口接收侧控制接口：消费单字符命令并映射成本地动作，
 * 不向串口发送任何响应。
 */
#ifndef SERIAL_CONTROL_H
#define SERIAL_CONTROL_H                                /* 防止 serial_control.h 被重复包含。 */

typedef void (*SerialFlashEraseHandler)(void);             /* 串口 e 命令的 Flash 擦除处理器，可由 RTOS 任务异步接管。 */

void serial_control_set_flash_erase_handler(SerialFlashEraseHandler handler); /* 设置 Flash 擦除处理器；传入 0 恢复同步默认实现。 */
void serial_control_poll(void);                            /* 读取一个待处理串口字符并执行对应本地控制。 */

#endif
