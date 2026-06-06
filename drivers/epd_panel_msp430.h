/*
 * epd_panel_msp430.h
 * MSP430F5529 墨水屏面板驱动接口：封装 GPIO、软件 SPI、BUSY 等待、
 * SSD1673/备用控制器初始化和帧缓冲提交细节。
 */
#ifndef EPD_PANEL_MSP430_H
#define EPD_PANEL_MSP430_H                              /* 防止 epd_panel_msp430.h 被重复包含。 */

#include <stdint.h>

void epd_panel_ssd_init_controller_and_clear(void);      /* 初始化 SSD1673 控制器并建立白底局刷基准。 */
void epd_panel_alt_init(void);                           /* 初始化备用墨水屏控制器。 */
uint8_t epd_panel_ssd_flush_partial(const uint8_t *buf); /* 将 SSD1673 帧缓冲以局部刷新方式提交到屏幕。 */
uint8_t epd_panel_alt_flush(const uint8_t *buf);         /* 将备用驱动帧缓冲提交到屏幕。 */

#endif
