/*
 * HAL_SDCard.h
 * SD 卡 SPI 硬件适配接口：隐藏 UCB1 和片选 GPIO 细节，
 * 供 FatFs 底层扇区驱动调用。
 */
#ifndef HAL_SDCARD_H
#define HAL_SDCARD_H                                 /* 防止 HAL_SDCard.h 被重复包含。 */

#include <stdint.h>

unsigned char SDCard_init(void);                     /* 初始化 SD 卡 SPI 引脚、CS 引脚和 UCB1 慢速 SPI。 */
void SDCard_fastMode(void);                          /* 初始化完成后切换到高速 SPI 读写模式。 */
uint8_t SDCard_readFrame(uint8_t *pBuffer, uint16_t size); /* 通过 SPI 连续读取指定字节数，失败返回 0。 */
uint8_t SDCard_sendFrame(const uint8_t *pBuffer, uint16_t size); /* 通过 SPI 连续发送指定字节数，失败返回 0。 */
void SDCard_setCSHigh(void);                         /* 拉高 SD 卡片选信号，释放 SD 卡。 */
void SDCard_setCSLow(void);                          /* 拉低 SD 卡片选信号，选择 SD 卡。 */

#endif
