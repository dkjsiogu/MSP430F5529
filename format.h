/*
 * format.h
 * 小型字符串格式化工具：在不依赖 printf 的情况下拼接字符串、
 * 无符号整数和 0.1 摄氏度温度文本，供串口和墨水屏共用。
 */
#ifndef FORMAT_H
#define FORMAT_H                                        /* 防止 format.h 被重复包含。 */

#include "app_types.h"

char *append_str(char *p, const char *s);                  /* 将字符串 s 追加到 p，并返回新的写入位置。 */
char *append_u16(char *p, uint16_t v);                     /* 将 uint16 十进制文本追加到 p，并返回新位置。 */
char *append_t10(char *p, int16_t t10);                    /* 将 0.1 摄氏度温度格式化为 x.y 或 --.- 并追加。 */

#endif
