/*
 * captouch.h
 * Pocket Kit Pad1/Pad2 电容触摸底层驱动：使用 Comparator B 和 P6.0/P6.1
 * RC 振荡测频，向应用层提供触摸按下状态。
 */
#ifndef CAPTOUCH_H
#define CAPTOUCH_H                                      /* 防止 captouch.h 被重复包含。 */

#include <stdint.h>

void captouch_init(void);                               /* 初始化 Comparator B 和触摸基线。 */
uint16_t captouch_read_channel(uint8_t channel);        /* 读取一个触摸通道的振荡耗时计数。 */
uint8_t captouch_read_pressed_mask(void);               /* 读取 Pad1/Pad2 当前触摸状态位。 */

#endif
