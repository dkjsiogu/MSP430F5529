/*
 * app_config.h
 * 全局硬件和应用配置：时钟、采样周期、报警阈值、Flash 日志区、
 * 墨水屏引脚、按键引脚和蜂鸣器参数都集中放在这里。
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H                                      /* 防止 app_config.h 被重复包含。 */

#include <msp430.h>
#include <stdint.h>

#define MCLK_HZ                     8000000UL /* 主系统时钟频率，单位 Hz。 */
#define SMCLK_HZ                    8000000UL /* 子系统时钟频率，供 UART、I2C、蜂鸣器等外设使用。 */
#define BOARD_TICK_HZ               100u      /* 板级系统节拍频率，100Hz 表示 10ms 一次中断。 */
#define BOARD_TICKS_PER_SECOND      100u      /* 1 秒对应的 10ms 节拍数量。 */
#define SAMPLE_INTERVAL_SECONDS     3u        /* 默认温度采样间隔，单位秒。 */
#define SAMPLE_INTERVAL_MIN_SECONDS 1u        /* 设置界面允许的最小采样间隔，单位秒。 */
#define SAMPLE_INTERVAL_MAX_SECONDS 60u       /* 设置界面允许的最大采样间隔，单位秒。 */
#define SAMPLE_INTERVAL_STEP_SECONDS 1u       /* 每次按键调整采样间隔的步进，单位秒。 */

#define ALERT_THRESHOLD_T10         500       /* 默认报警温度阈值，单位 0.1 摄氏度，500 表示 50.0 摄氏度。 */
#define ALERT_THRESHOLD_MIN_T10     -400      /* 报警温度阈值下限，单位 0.1 摄氏度。 */
#define ALERT_THRESHOLD_MAX_T10     1250      /* 报警温度阈值上限，单位 0.1 摄氏度。 */
#define ALERT_THRESHOLD_STEP_T10    10        /* 每次按键调整报警阈值的步进，单位 0.1 摄氏度，10 表示 1.0 摄氏度。 */

#define STORAGE_LIMIT_DEFAULT       200u      /* 默认 Flash 历史记录保存条数。 */
#define STORAGE_LIMIT_MIN           10u       /* 设置界面允许的最小历史记录保存条数。 */
#define STORAGE_LIMIT_MAX           990u      /* 设置界面允许的最大历史记录保存条数。 */
#define STORAGE_LIMIT_STEP          10u       /* 每次按键调整历史记录条数的步进。 */

#define ALARM_DURATION_SECONDS      3u        /* 默认蜂鸣器报警持续时间，单位秒。 */
#define ALARM_DURATION_MIN_SECONDS  1u        /* 设置界面允许的最短报警持续时间，单位秒。 */
#define ALARM_DURATION_MAX_SECONDS  30u       /* 设置界面允许的最长报警持续时间，单位秒。 */
#define ALARM_DURATION_STEP_SECONDS 1u        /* 每次按键调整报警持续时间的步进，单位秒。 */

#define SETTINGS_FLASH_START        0x1800u   /* Info Flash 设置记录区起始地址。 */
#define SETTINGS_FLASH_END          0x1880u   /* Info Flash 设置记录区结束地址，不包含该地址。 */
#define SETTINGS_MAGIC              0x51A7u   /* 设置记录有效性标识，用于区分空白或无效 Flash 数据。 */

/* NTC 默认参数：10k NTC，beta=3950，10k 上拉到 VCC，NTC 接 GND。 */
#define NTC_ADC_INCH                ADC12INCH_5 /* NTC 热敏电阻使用的 ADC12 输入通道。 */
#define NTC_PORT_DIR                P6DIR       /* NTC ADC 引脚所在端口方向寄存器。 */
#define NTC_PORT_REN                P6REN       /* NTC ADC 引脚所在端口上下拉使能寄存器。 */
#define NTC_PORT_OUT                P6OUT       /* NTC ADC 引脚所在端口输出寄存器。 */
#define NTC_PORT_SEL                P6SEL       /* NTC ADC 引脚所在端口功能选择寄存器。 */
#define NTC_PORT_BIT                BIT5        /* NTC ADC 对应的 P6.5 引脚位。 */
#define NTC_PULLUP_TO_VCC           1           /* NTC 分压结构标志，1 表示上拉电阻接 VCC、NTC 接 GND。 */

/* Pocket Kit S1/S2：P1.2/P1.3，S3/S4：P2.3/P2.6，内部上拉，按下为低电平。 */
#define BUTTON_PORT_IN              P1IN      /* S1/S2 所在 P1 端口输入寄存器。 */
#define BUTTON_PORT_DIR             P1DIR     /* S1/S2 所在 P1 端口方向寄存器。 */
#define BUTTON_PORT_REN             P1REN     /* S1/S2 所在 P1 端口上下拉使能寄存器。 */
#define BUTTON_PORT_OUT             P1OUT     /* S1/S2 所在 P1 端口输出寄存器，用于配置上拉。 */
#define BUTTON_PORT_SEL             P1SEL     /* S1/S2 所在 P1 端口功能选择寄存器。 */
#define BUTTON_PORT_IE              P1IE      /* S1/S2 所在 P1 端口中断使能寄存器。 */
#define BUTTON_PORT_IES             P1IES     /* S1/S2 所在 P1 端口中断边沿选择寄存器。 */
#define BUTTON_PORT_IFG             P1IFG     /* S1/S2 所在 P1 端口中断标志寄存器。 */
#define BUTTON_S1_BIT               BIT2      /* Pocket Kit S1 按键对应 P1.2。 */
#define BUTTON_S2_BIT               BIT3      /* Pocket Kit S2 按键对应 P1.3。 */

#define BUTTON2_PORT_IN             P2IN      /* S3/S4 所在 P2 端口输入寄存器。 */
#define BUTTON2_PORT_DIR            P2DIR     /* S3/S4 所在 P2 端口方向寄存器。 */
#define BUTTON2_PORT_REN            P2REN     /* S3/S4 所在 P2 端口上下拉使能寄存器。 */
#define BUTTON2_PORT_OUT            P2OUT     /* S3/S4 所在 P2 端口输出寄存器，用于配置上拉。 */
#define BUTTON2_PORT_SEL            P2SEL     /* S3/S4 所在 P2 端口功能选择寄存器。 */
#define BUTTON2_PORT_IE             P2IE      /* S3/S4 所在 P2 端口中断使能寄存器。 */
#define BUTTON2_PORT_IES            P2IES     /* S3/S4 所在 P2 端口中断边沿选择寄存器。 */
#define BUTTON2_PORT_IFG            P2IFG     /* S3/S4 所在 P2 端口中断标志寄存器。 */
#define BUTTON_S3_BIT               BIT3      /* Pocket Kit S3 按键对应 P2.3。 */
#define BUTTON_S4_BIT               BIT6      /* Pocket Kit S4 按键对应 P2.6。 */
#define BUTTON_DEBOUNCE_MS          12u       /* 按键去抖等待时间，单位毫秒。 */
#define BUTTON_REPEAT_GUARD_TICKS   8u        /* 按键触发后 80ms 内忽略重复边沿，过滤释放抖动。 */
#define BUTTON_BEEP_MS              180u      /* 预留的按键蜂鸣反馈时长，单位毫秒。 */
#define SETTINGS_SAVE_DELAY_TICKS   80u       /* 设置修改后空闲约 0.8 秒再写 Flash，减少连续按键阻塞。 */

/* 片内主 Flash 日志保留区，需要和 lnk_msp430f5529.cmd 保持一致。 */
#define FLASH_LOG_START             0xC000u   /* 片内主 Flash 历史记录区起始地址。 */
#define FLASH_LOG_END               0xFE00u   /* 片内主 Flash 历史记录区结束地址，不包含该地址。 */
#define FLASH_SEGMENT_SIZE          0x0200u   /* MSP430F5529 主 Flash 单段大小，单位字节。 */
#define RECORD_MAGIC                0xA55Au   /* 温度历史记录有效性标识。 */

/* 墨水屏原生 RAM 为 128 x 250，界面坐标旋转为 250 x 122。 */
#define EPD_SCREEN_W                250u      /* SSD1673 墨水屏渲染坐标宽度，单位像素。 */
#define EPD_SCREEN_H                122u      /* SSD1673 墨水屏渲染坐标高度，单位像素。 */
#define EPD_RAM_W_BYTES             16u       /* SSD1673 原生 RAM 每行字节数，128 像素 / 8。 */
#define EPD_RAM_H                   250u      /* SSD1673 原生 RAM 行数。 */
#define EPD_BUF_SIZE                (EPD_RAM_W_BYTES * EPD_RAM_H) /* SSD1673 整屏帧缓冲字节数。 */
#define EPD_ALT_SCREEN_W            172u      /* 备用墨水屏驱动的渲染坐标宽度，单位像素。 */
#define EPD_ALT_SCREEN_H            144u      /* 备用墨水屏驱动的渲染坐标高度，单位像素。 */
#define EPD_ALT_RAM_W_BYTES         18u       /* 备用墨水屏驱动原生 RAM 每行字节数。 */
#define EPD_ALT_RAM_H               172u      /* 备用墨水屏驱动原生 RAM 行数。 */
#define EPD_ALT_BUF_SIZE            (EPD_ALT_RAM_W_BYTES * EPD_ALT_RAM_H) /* 备用墨水屏整屏帧缓冲字节数。 */
#define HISTORY_ROWS_ON_EPD         5u        /* 历史记录页面每屏显示的记录行数。 */

#define EPD_BUSY_IN                 P2IN      /* 墨水屏 BUSY 引脚所在端口输入寄存器。 */
#define EPD_BUSY_DIR                P2DIR     /* 墨水屏 BUSY 引脚所在端口方向寄存器。 */
#define EPD_BUSY_REN                P2REN     /* 墨水屏 BUSY 引脚所在端口上下拉使能寄存器。 */
#define EPD_BUSY_OUT                P2OUT     /* 墨水屏 BUSY 引脚所在端口输出寄存器。 */
#define EPD_BUSY_SEL                P2SEL     /* 墨水屏 BUSY 引脚所在端口功能选择寄存器。 */
#define EPD_BUSY_BIT                BIT2      /* 墨水屏 BUSY 信号对应 P2.2。 */

#define EPD_RST_SEL                 P1SEL     /* 墨水屏 RST 引脚所在端口功能选择寄存器。 */
#define EPD_RST_DIR                 P1DIR     /* 墨水屏 RST 引脚所在端口方向寄存器。 */
#define EPD_RST_OUT                 P1OUT     /* 墨水屏 RST 引脚所在端口输出寄存器。 */
#define EPD_RST_BIT                 BIT4      /* 墨水屏 RST 信号对应 P1.4。 */

#define EPD_DC_SEL                  P3SEL     /* 墨水屏 D/C 引脚所在端口功能选择寄存器。 */
#define EPD_DC_DIR                  P3DIR     /* 墨水屏 D/C 引脚所在端口方向寄存器。 */
#define EPD_DC_OUT                  P3OUT     /* 墨水屏 D/C 引脚所在端口输出寄存器。 */
#define EPD_DC_BIT                  BIT4      /* 墨水屏 D/C 信号对应 P3.4。 */

#define EPD_CS_SEL                  P3SEL     /* 墨水屏 CS 引脚所在端口功能选择寄存器。 */
#define EPD_CS_DIR                  P3DIR     /* 墨水屏 CS 引脚所在端口方向寄存器。 */
#define EPD_CS_OUT                  P3OUT     /* 墨水屏 CS 引脚所在端口输出寄存器。 */
#define EPD_CS_BIT                  BIT2      /* 墨水屏 CS 信号对应 P3.2。 */

#define EPD_SDI_SEL                 P3SEL     /* 墨水屏 SDI/MOSI 引脚所在端口功能选择寄存器。 */
#define EPD_SDI_DIR                 P3DIR     /* 墨水屏 SDI/MOSI 引脚所在端口方向寄存器。 */
#define EPD_SDI_OUT                 P3OUT     /* 墨水屏 SDI/MOSI 引脚所在端口输出寄存器。 */
#define EPD_SDI_BIT                 BIT3      /* 墨水屏 SDI/MOSI 信号对应 P3.3。 */

#define EPD_CLK_SEL                 P2SEL     /* 墨水屏 SPI CLK 引脚所在端口功能选择寄存器。 */
#define EPD_CLK_DIR                 P2DIR     /* 墨水屏 SPI CLK 引脚所在端口方向寄存器。 */
#define EPD_CLK_OUT                 P2OUT     /* 墨水屏 SPI CLK 引脚所在端口输出寄存器。 */
#define EPD_CLK_BIT                 BIT7      /* 墨水屏 SPI CLK 信号对应 P2.7。 */
#define EPD_BUSY_TIMEOUT_MS         8000u     /* 等待墨水屏 BUSY 释放的最长时间，单位毫秒。 */
#define EPD_BUSY_START_TIMEOUT_MS   40u       /* 触发刷新后等待 BUSY 拉高的最长时间，单位毫秒。 */
#define EPD_FULL_POST_UPDATE_MS     1500u     /* 全屏刷新完成后的稳定等待时间，单位毫秒。 */
#define EPD_PARTIAL_POST_UPDATE_MS  60u       /* 局部刷新完成后的稳定等待时间，单位毫秒。 */
#define EPD_NO_BUSY_FALLBACK_MS     120u      /* 未检测到 BUSY 变化时的兜底等待时间，单位毫秒。 */
#define EPD_AUTO_FRAME_TICKS        12u       /* 自动温度页两帧之间的节拍间隔，约 120ms。 */
#define EPD_HISTORY_SCROLL_TICKS    160u      /* 历史页滚动播放间隔，约 1.6 秒，并重新读取 Flash。 */
#define EPD_STARTUP_SETTLE_MS       500u      /* 墨水屏上电后开始初始化前的稳定等待时间，单位毫秒。 */
#define EPD_RESET_PRE_MS            50u       /* 墨水屏复位前保持 RST 高电平的时间，单位毫秒。 */
#define EPD_RESET_LOW_MS            100u      /* 墨水屏复位时 RST 低电平保持时间，单位毫秒。 */
#define EPD_RESET_HIGH_MS           200u      /* 墨水屏复位释放后等待控制器稳定的时间，单位毫秒。 */
#define EPD_UPDATE_CTRL_FULL        0xC7u     /* SSD1673 全屏刷新控制字。 */
#define EPD_UPDATE_CTRL_PARTIAL     0xC4u     /* SSD1673 局部刷新控制字。 */
#define EPD_DRIVER_SSD1673          0u        /* 墨水屏驱动选择值：SSD1673 驱动。 */
#define EPD_DRIVER_SPD2701          1u        /* 墨水屏驱动选择值：备用 SPD2701 类驱动。 */

#define BUZZER_DIR                  P3DIR     /* 蜂鸣器引脚所在端口方向寄存器。 */
#define BUZZER_OUT                  P3OUT     /* 蜂鸣器引脚所在端口输出寄存器。 */
#define BUZZER_REN                  P3REN     /* 蜂鸣器引脚所在端口上下拉使能寄存器。 */
#define BUZZER_SEL                  P3SEL     /* 蜂鸣器引脚所在端口功能选择寄存器。 */
#define BUZZER_BIT                  BIT6      /* 蜂鸣器驱动信号对应 P3.6。 */
#define BUZZER_FREQ_HZ              5000u     /* 无源蜂鸣器驱动方波频率，单位 Hz。 */
#define BUZZER_HALF_PERIOD_TICKS    ((uint16_t)(SMCLK_HZ / (BUZZER_FREQ_HZ * 2u))) /* 蜂鸣器半周期对应的 SMCLK 计数。 */

#endif
