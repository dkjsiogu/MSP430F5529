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
void adc_dma_init(void);                                   /* 启动 ADC12 序列+Timer_B0 触发+DMA 搬移的自动采样管线。 */
void adc_dma_start(void);                                  /* 启动采集：重新装填序列+DMA 并启动 Timer_B0。 */
void adc_dma_stop(void);                                   /* 停止采集：停掉 Timer_B0 触发并关闭 ADC12 转换。 */
uint8_t adc_dma_running(void);                             /* 返回采集运行状态：1 运行、0 停止。 */
uint8_t adc_dma_batch_take(TempSample *out);               /* 取走 5 轮双通道 DMA 批量并合成 TempSample，无批次返回 0。 */

#endif
