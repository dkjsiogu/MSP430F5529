/*
 * display_config.h
 * 显示服务和墨水屏面板驱动共享配置：屏幕几何、帧缓冲大小、
 * 刷新节奏、BUSY 超时、复位时序和控制器刷新控制字。
 */
#ifndef DISPLAY_CONFIG_H
#define DISPLAY_CONFIG_H                                 /* 防止 display_config.h 被重复包含。 */

#include <stdint.h>

#define EPD_SCREEN_W                  250u               /* SSD1673 渲染坐标宽度，单位像素。 */
#define EPD_SCREEN_H                  122u               /* SSD1673 渲染坐标高度，单位像素。 */
#define EPD_RAM_W_BYTES               16u                /* SSD1673 原生 RAM 每行字节数。 */
#define EPD_RAM_H                     250u               /* SSD1673 原生 RAM 行数。 */
#define EPD_BUF_SIZE                  (EPD_RAM_W_BYTES * EPD_RAM_H) /* SSD1673 整屏帧缓冲字节数。 */

#define EPD_ALT_SCREEN_W              172u               /* 备用屏渲染坐标宽度，单位像素。 */
#define EPD_ALT_SCREEN_H              144u               /* 备用屏渲染坐标高度，单位像素。 */
#define EPD_ALT_RAM_W_BYTES           18u                /* 备用屏原生 RAM 每行字节数。 */
#define EPD_ALT_RAM_H                 172u               /* 备用屏原生 RAM 行数。 */
#define EPD_ALT_BUF_SIZE              (EPD_ALT_RAM_W_BYTES * EPD_ALT_RAM_H) /* 备用屏整屏帧缓冲字节数。 */

#define EPD_BUSY_TIMEOUT_MS           8000u              /* 等待墨水屏 BUSY 释放的最长时间，单位毫秒。 */
#define EPD_BUSY_START_TIMEOUT_MS     40u                /* 触发刷新后等待 BUSY 拉高的最长时间，单位毫秒。 */
#define EPD_FULL_POST_UPDATE_MS       1500u              /* 全屏刷新完成后的稳定等待时间，单位毫秒。 */
#define EPD_PARTIAL_POST_UPDATE_MS    60u                /* 局部刷新完成后的稳定等待时间，单位毫秒。 */
#define EPD_NO_BUSY_FALLBACK_MS       120u               /* 未检测到 BUSY 变化时的兜底等待时间，单位毫秒。 */

#define EPD_RENDER_MIN_TICKS          1u                 /* 两次实际提交的最小间隔，单位 10ms。 */
#define EPD_AUTO_FRAME_TICKS          10u                /* 自动温度页和 GIF 帧间隔，约 100ms。 */
#define EPD_HISTORY_SCROLL_TICKS      160u               /* 历史页滚动播放间隔，约 1.6 秒。 */

#define EPD_STARTUP_SETTLE_MS         500u               /* 墨水屏上电后初始化前的稳定等待时间，单位毫秒。 */
#define EPD_RESET_PRE_MS              50u                /* 墨水屏复位前保持 RST 高电平的时间，单位毫秒。 */
#define EPD_RESET_LOW_MS              100u               /* 墨水屏复位时 RST 低电平保持时间，单位毫秒。 */
#define EPD_RESET_HIGH_MS             200u               /* 墨水屏复位释放后等待控制器稳定的时间，单位毫秒。 */

#define EPD_UPDATE_CTRL_FULL          0xC7u              /* SSD1673 全屏刷新控制字。 */
#define EPD_UPDATE_CTRL_PARTIAL       0xC4u              /* SSD1673 局部刷新控制字。 */

#define EPD_DRIVER_SSD1673            0u                 /* 显示服务驱动选择值：SSD1673。 */
#define EPD_DRIVER_SPD2701            1u                 /* 显示服务驱动选择值：备用 SPD2701 类驱动。 */

#endif
