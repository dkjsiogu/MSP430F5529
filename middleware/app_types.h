/*
 * app_types.h
 * 跨层公共数据类型：温度样本、Flash 历史记录和 NTC 查表点。
 * 这些类型描述数据契约，不包含应用策略、寄存器或平台地址。
 */
#ifndef APP_TYPES_H
#define APP_TYPES_H                                      /* 防止 app_types.h 被重复包含。 */

#include <stdint.h>

#define INVALID_T10                 ((int16_t)-32768)    /* 无效温度标记，单位仍按 0.1 摄氏度存储。 */

#define FLAG_DIE_OK                 0x0001u              /* 片内 DIE 温度采集有效标志。 */
#define FLAG_NTC_OK                 0x0002u              /* NTC 热敏电阻温度采集有效标志。 */
#define FLAG_TMP_LOCAL_OK           0x0004u              /* TMP421 本地温度采集有效标志。 */

typedef struct {
    uint32_t ohms;
    int16_t t10;
} NtcPoint;

typedef struct {
    int16_t die_t10;
    int16_t ntc_t10;
    int16_t tmp_local_t10;
    int16_t reserved_t10;                                /* 保留字段，用于保持样本和 Flash 记录布局稳定。 */
    uint16_t flags;
} TempSample;

typedef struct {
    uint16_t magic;
    uint16_t seq;
    int16_t die_t10;
    int16_t ntc_t10;
    int16_t tmp_local_t10;
    int16_t reserved_t10;                                /* 保留字段，用于兼容已有 Flash 历史记录格式。 */
    uint16_t flags;
    uint16_t crc;
} TempRecord;

#endif
