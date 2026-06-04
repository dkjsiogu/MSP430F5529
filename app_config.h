/*
 * app_config.h
 * 全局硬件和应用配置：时钟、采样周期、报警阈值、Flash 日志区、
 * 墨水屏引脚、按键引脚和蜂鸣器参数都集中放在这里。
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <msp430.h>
#include <stdint.h>

#define MCLK_HZ                     8000000UL
#define SMCLK_HZ                    8000000UL
#define BOARD_TICK_HZ               100u
#define BOARD_TICKS_PER_SECOND      100u
#define SAMPLE_INTERVAL_SECONDS     3u
#define SAMPLE_INTERVAL_MIN_SECONDS 1u
#define SAMPLE_INTERVAL_MAX_SECONDS 60u
#define SAMPLE_INTERVAL_STEP_SECONDS 1u

#define ALERT_THRESHOLD_T10         500       /* 50.0 摄氏度 */
#define ALERT_THRESHOLD_MIN_T10     -400
#define ALERT_THRESHOLD_MAX_T10     1250
#define ALERT_THRESHOLD_STEP_T10    10        /* 1.0 摄氏度 */

#define STORAGE_LIMIT_DEFAULT       200u
#define STORAGE_LIMIT_MIN           10u
#define STORAGE_LIMIT_MAX           990u
#define STORAGE_LIMIT_STEP          10u

#define ALARM_DURATION_SECONDS      3u
#define ALARM_DURATION_MIN_SECONDS  1u
#define ALARM_DURATION_MAX_SECONDS  30u
#define ALARM_DURATION_STEP_SECONDS 1u

#define SETTINGS_FLASH_START        0x1800u
#define SETTINGS_FLASH_END          0x1880u
#define SETTINGS_MAGIC              0x51A7u

/* NTC 默认参数：10k NTC，beta=3950，10k 上拉到 VCC，NTC 接 GND。 */
#define NTC_ADC_INCH                ADC12INCH_5
#define NTC_PORT_DIR                P6DIR
#define NTC_PORT_REN                P6REN
#define NTC_PORT_OUT                P6OUT
#define NTC_PORT_SEL                P6SEL
#define NTC_PORT_BIT                BIT5
#define NTC_PULLUP_TO_VCC           1

/* Pocket Kit S1/S2：P1.2/P1.3，S3/S4：P2.3/P2.6，内部上拉，按下为低电平。 */
#define BUTTON_PORT_IN              P1IN
#define BUTTON_PORT_DIR             P1DIR
#define BUTTON_PORT_REN             P1REN
#define BUTTON_PORT_OUT             P1OUT
#define BUTTON_PORT_SEL             P1SEL
#define BUTTON_PORT_IE              P1IE
#define BUTTON_PORT_IES             P1IES
#define BUTTON_PORT_IFG             P1IFG
#define BUTTON_S1_BIT               BIT2
#define BUTTON_S2_BIT               BIT3

#define BUTTON2_PORT_IN             P2IN
#define BUTTON2_PORT_DIR            P2DIR
#define BUTTON2_PORT_REN            P2REN
#define BUTTON2_PORT_OUT            P2OUT
#define BUTTON2_PORT_SEL            P2SEL
#define BUTTON2_PORT_IE             P2IE
#define BUTTON2_PORT_IES            P2IES
#define BUTTON2_PORT_IFG            P2IFG
#define BUTTON_S3_BIT               BIT3
#define BUTTON_S4_BIT               BIT6
#define BUTTON_DEBOUNCE_MS          12u
#define BUTTON_REPEAT_GUARD_TICKS   8u        /* 按键触发后 80ms 内忽略重复边沿，过滤释放抖动。 */
#define BUTTON_BEEP_MS              180u
#define SETTINGS_SAVE_DELAY_TICKS   80u       /* 设置修改后空闲约 0.8 秒再写 Flash。 */

/* 片内主 Flash 日志保留区，需要和 lnk_msp430f5529.cmd 保持一致。 */
#define FLASH_LOG_START             0xC000u
#define FLASH_LOG_END               0xFE00u
#define FLASH_SEGMENT_SIZE          0x0200u
#define RECORD_MAGIC                0xA55Au

/* 墨水屏原生 RAM 为 128 x 250，界面坐标旋转为 250 x 122。 */
#define EPD_SCREEN_W                250u
#define EPD_SCREEN_H                122u
#define EPD_RAM_W_BYTES             16u
#define EPD_RAM_H                   250u
#define EPD_BUF_SIZE                (EPD_RAM_W_BYTES * EPD_RAM_H)
#define EPD_ALT_SCREEN_W            172u
#define EPD_ALT_SCREEN_H            144u
#define EPD_ALT_RAM_W_BYTES         18u
#define EPD_ALT_RAM_H               172u
#define EPD_ALT_BUF_SIZE            (EPD_ALT_RAM_W_BYTES * EPD_ALT_RAM_H)
#define HISTORY_ROWS_ON_EPD         5u

#define EPD_BUSY_IN                 P2IN
#define EPD_BUSY_DIR                P2DIR
#define EPD_BUSY_REN                P2REN
#define EPD_BUSY_OUT                P2OUT
#define EPD_BUSY_SEL                P2SEL
#define EPD_BUSY_BIT                BIT2

#define EPD_RST_SEL                 P1SEL
#define EPD_RST_DIR                 P1DIR
#define EPD_RST_OUT                 P1OUT
#define EPD_RST_BIT                 BIT4

#define EPD_DC_SEL                  P3SEL
#define EPD_DC_DIR                  P3DIR
#define EPD_DC_OUT                  P3OUT
#define EPD_DC_BIT                  BIT4

#define EPD_CS_SEL                  P3SEL
#define EPD_CS_DIR                  P3DIR
#define EPD_CS_OUT                  P3OUT
#define EPD_CS_BIT                  BIT2

#define EPD_SDI_SEL                 P3SEL
#define EPD_SDI_DIR                 P3DIR
#define EPD_SDI_OUT                 P3OUT
#define EPD_SDI_BIT                 BIT3

#define EPD_CLK_SEL                 P2SEL
#define EPD_CLK_DIR                 P2DIR
#define EPD_CLK_OUT                 P2OUT
#define EPD_CLK_BIT                 BIT7
#define EPD_BUSY_TIMEOUT_MS         8000u
#define EPD_BUSY_START_TIMEOUT_MS   40u
#define EPD_FULL_POST_UPDATE_MS     1500u
#define EPD_PARTIAL_POST_UPDATE_MS  60u
#define EPD_NO_BUSY_FALLBACK_MS     120u
#define EPD_AUTO_FRAME_TICKS        12u       /* 自动页两帧之间留出约 120ms 给按键和采样。 */
#define EPD_STARTUP_SETTLE_MS       500u
#define EPD_RESET_PRE_MS            50u
#define EPD_RESET_LOW_MS            100u
#define EPD_RESET_HIGH_MS           200u
#define EPD_UPDATE_CTRL_FULL        0xC7u
#define EPD_UPDATE_CTRL_PARTIAL     0xC4u
#define EPD_DRIVER_SSD1673          0u
#define EPD_DRIVER_SPD2701          1u

#define BUZZER_DIR                  P3DIR
#define BUZZER_OUT                  P3OUT
#define BUZZER_REN                  P3REN
#define BUZZER_SEL                  P3SEL
#define BUZZER_BIT                  BIT6
#define BUZZER_FREQ_HZ              5000u
#define BUZZER_HALF_PERIOD_TICKS    ((uint16_t)(SMCLK_HZ / (BUZZER_FREQ_HZ * 2u)))

#endif
