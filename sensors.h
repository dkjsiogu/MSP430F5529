/*
 * sensors.h
 * 温度采集接口：封装 MSP430 片内温度、NTC ADC 和 TMP421 本地温度读取，
 * 给主循环提供统一的 TempSample 采样结果。
 */
#ifndef SENSORS_H
#define SENSORS_H                                       /* 防止 sensors.h 被重复包含。 */

#include "app_types.h"

void sensors_init(void);                                   /* 初始化 ADC、I2C 和 TMP421 传感器。 */
void collect_sample(TempSample *s);                        /* 采集 DIE、NTC、TMP421 本地温度到样本结构。 */
uint8_t tmp421_addr(void);                                 /* 返回已检测到的 TMP421 I2C 地址，未检测到返回 0。 */
uint8_t tmp421_detect(void);                               /* 扫描并确认 TMP421 是否在线，成功返回 1。 */

#endif
