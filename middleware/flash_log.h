/*
 * flash_log.h
 * 片内 Flash 历史记录接口：负责扫描、写入、擦除和读取温度记录，
 * 同时提供串口打印历史数据的辅助函数。
 */
#ifndef FLASH_LOG_H
#define FLASH_LOG_H                                     /* 防止 flash_log.h 被重复包含。 */

#include "app_types.h"

void flash_scan(void);                                     /* 启动时扫描 Flash 日志区，定位下一条写入位置。 */
void flash_log_sample(const TempSample *s);                /* 将一次温度样本写入片内 Flash 历史记录。 */
void flash_erase_log(void);                                /* 擦除整个 Flash 历史记录区并重置序号。 */
uint16_t history_count(void);                              /* 返回当前有效历史记录条数。 */
uint8_t history_get(uint16_t index, TempRecord *out);      /* 读取指定下标的历史记录，成功返回 1。 */
uint16_t flash_next_seq(void);                             /* 返回下一条历史记录的序号，供主界面显示采集进度。 */

#endif
