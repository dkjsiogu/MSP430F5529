/*
 * app_types.h
 * 全工程共用的数据类型和温度有效标志定义：温度样本、Flash 历史记录、
 * NTC 查表点等结构体都放在这里，避免各模块重复定义。
 */
#ifndef APP_TYPES_H
#define APP_TYPES_H

#include "app_config.h"

#define INVALID_T10                 ((int16_t)-32768)

#define FLAG_DIE_OK                 0x0001u
#define FLAG_NTC_OK                 0x0002u
#define FLAG_TMP_LOCAL_OK           0x0004u

typedef struct {
    uint16_t adc;
    int16_t t10;
} NtcPoint;

typedef struct {
    int16_t die_t10;
    int16_t ntc_t10;
    int16_t tmp_local_t10;
    int16_t reserved_t10;                                  /* 保留字段，用于保持样本和 Flash 记录布局稳定。 */
    uint16_t flags;
} TempSample;

typedef struct {
    uint16_t magic;
    uint16_t seq;
    int16_t die_t10;
    int16_t ntc_t10;
    int16_t tmp_local_t10;
    int16_t reserved_t10;                                  /* 保留字段，用于兼容已有 Flash 历史记录格式。 */
    uint16_t flags;
    uint16_t crc;
} TempRecord;

#endif
