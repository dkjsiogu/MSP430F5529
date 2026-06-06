/*
 * platform_config.h
 * MSP430F5529 Pocket Kit 板级配置：时钟、引脚、片内 Flash 地址、
 * ADC 通道、SPI/UART/GPIO 寄存器和驱动层参数都集中在这里。
 */
#ifndef PLATFORM_CONFIG_H
#define PLATFORM_CONFIG_H                               /* 防止 platform_config.h 被重复包含。 */

#include <msp430.h>
#include <stdint.h>

#define MCLK_HZ                       16000000UL        /* 主系统时钟频率，单位 Hz。 */
#define SMCLK_HZ                      16000000UL        /* 子系统时钟频率，供 UART、I2C、SPI 和蜂鸣器使用。 */

#define SETTINGS_FLASH_START          0x1800u           /* Info Flash 设置记录区起始地址。 */
#define SETTINGS_FLASH_END            0x1880u           /* Info Flash 设置记录区结束地址，不包含该地址。 */

#define FLASH_LOG_START               0xC000u           /* 片内主 Flash 历史记录区起始地址。 */
#define FLASH_LOG_END                 0xFE00u           /* 片内主 Flash 历史记录区结束地址，不包含该地址。 */
#define FLASH_SEGMENT_SIZE            0x0200u           /* MSP430F5529 主 Flash 单段大小，单位字节。 */

/* NTC：10k NTC，15k 固定电阻接 VCC，NTC 接 GND，P6.5/A5 采样分压点。 */
#define NTC_ADC_INCH                  ADC12INCH_5       /* NTC 使用的 ADC12 输入通道。 */
#define NTC_PORT_DIR                  P6DIR             /* NTC ADC 引脚所在端口方向寄存器。 */
#define NTC_PORT_REN                  P6REN             /* NTC ADC 引脚所在端口上下拉使能寄存器。 */
#define NTC_PORT_OUT                  P6OUT             /* NTC ADC 引脚所在端口输出寄存器。 */
#define NTC_PORT_SEL                  P6SEL             /* NTC ADC 引脚所在端口功能选择寄存器。 */
#define NTC_PORT_BIT                  BIT5              /* NTC ADC 对应 P6.5。 */
#define NTC_SERIES_RESISTOR_OHMS      15000UL           /* NTC 分压固定电阻阻值，单位欧姆。 */
#define NTC_PULLUP_TO_VCC             1                 /* 1 表示固定电阻上拉到 VCC，NTC 接 GND。 */

/* Pocket Kit Pad1/Pad2：P6.0/P6.1 Comparator B 电容触摸输入。 */
#define CAP_TOUCH_PORT_DIR            P6DIR             /* 触摸 Pad 所在 P6 端口方向寄存器。 */
#define CAP_TOUCH_PORT_REN            P6REN             /* 触摸 Pad 所在 P6 端口上下拉使能寄存器。 */
#define CAP_TOUCH_PORT_OUT            P6OUT             /* 触摸 Pad 所在 P6 端口输出寄存器。 */
#define CAP_TOUCH_PORT_SEL            P6SEL             /* 触摸 Pad 所在 P6 端口功能选择寄存器。 */
#define CAP_TOUCH_ALL_BITS            (BIT0 | BIT1)     /* Pad1/Pad2 对应 P6.0/P6.1。 */
#define CAP_TOUCH_PAD1_CHANNEL        0u                /* Pad1 对应 Comparator B 输入通道 CB0。 */
#define CAP_TOUCH_PAD2_CHANNEL        1u                /* Pad2 对应 Comparator B 输入通道 CB1。 */
#define CAP_TOUCH_CHANNEL_COUNT       2u                /* 当前触摸输入通道数量。 */
#define CAP_TOUCH_OSC_CYCLES          10u               /* 每次测量累计的 RC 振荡边沿数量。 */
#define CAP_TOUCH_BASELINE_SAMPLES    4u                /* 上电建立触摸基线时每通道采样次数。 */
#define CAP_TOUCH_MEASURE_TIMEOUT     12000u            /* 单次触摸测量最大轮询次数。 */
#define CAP_TOUCH_MIN_THRESHOLD       210u              /* 触摸判定固定下限。 */
#define CAP_TOUCH_DELTA_MIN           45u               /* 相对基线触摸判定最小增量。 */
#define CAP_TOUCH_THRESHOLD_RATIO_DIV 3u                /* 相对基线门限增量比例。 */
#define CAP_TOUCH_RELEASE_HYSTERESIS  35u               /* 释放判定回差。 */
#define CAP_TOUCH_PRESS_SAMPLES       2u                /* 连续达到按下门限的采样次数。 */
#define CAP_TOUCH_RELEASE_SAMPLES     3u                /* 连续低于释放门限的采样次数。 */

/* Pocket Kit S1/S2：P1.2/P1.3；S3/S4：P2.3/P2.6，内部上拉，按下低电平。 */
#define BUTTON_PORT_IN                P1IN              /* S1/S2 所在 P1 端口输入寄存器。 */
#define BUTTON_PORT_DIR               P1DIR             /* S1/S2 所在 P1 端口方向寄存器。 */
#define BUTTON_PORT_REN               P1REN             /* S1/S2 所在 P1 端口上下拉使能寄存器。 */
#define BUTTON_PORT_OUT               P1OUT             /* S1/S2 所在 P1 端口输出寄存器。 */
#define BUTTON_PORT_SEL               P1SEL             /* S1/S2 所在 P1 端口功能选择寄存器。 */
#define BUTTON_PORT_IE                P1IE              /* S1/S2 所在 P1 端口中断使能寄存器。 */
#define BUTTON_PORT_IES               P1IES             /* S1/S2 所在 P1 端口中断边沿选择寄存器。 */
#define BUTTON_PORT_IFG               P1IFG             /* S1/S2 所在 P1 端口中断标志寄存器。 */
#define BUTTON_S1_BIT                 BIT2              /* Pocket Kit S1 对应 P1.2。 */
#define BUTTON_S2_BIT                 BIT3              /* Pocket Kit S2 对应 P1.3。 */

#define BUTTON2_PORT_IN               P2IN              /* S3/S4 所在 P2 端口输入寄存器。 */
#define BUTTON2_PORT_DIR              P2DIR             /* S3/S4 所在 P2 端口方向寄存器。 */
#define BUTTON2_PORT_REN              P2REN             /* S3/S4 所在 P2 端口上下拉使能寄存器。 */
#define BUTTON2_PORT_OUT              P2OUT             /* S3/S4 所在 P2 端口输出寄存器。 */
#define BUTTON2_PORT_SEL              P2SEL             /* S3/S4 所在 P2 端口功能选择寄存器。 */
#define BUTTON2_PORT_IE               P2IE              /* S3/S4 所在 P2 端口中断使能寄存器。 */
#define BUTTON2_PORT_IES              P2IES             /* S3/S4 所在 P2 端口中断边沿选择寄存器。 */
#define BUTTON2_PORT_IFG              P2IFG             /* S3/S4 所在 P2 端口中断标志寄存器。 */
#define BUTTON_S3_BIT                 BIT3              /* Pocket Kit S3 对应 P2.3。 */
#define BUTTON_S4_BIT                 BIT6              /* Pocket Kit S4 对应 P2.6。 */
#define BUTTON_DEBOUNCE_MS            12u               /* 按键去抖等待时间，单位毫秒。 */
#define BUTTON_REPEAT_GUARD_TICKS     8u                /* 80ms 内忽略重复边沿，过滤释放抖动。 */

/* 墨水屏板级引脚：SSD1673 主驱动和备用驱动共用软件 SPI GPIO。 */
#define EPD_BUSY_IN                   P2IN              /* EPD BUSY 输入寄存器。 */
#define EPD_BUSY_DIR                  P2DIR             /* EPD BUSY 方向寄存器。 */
#define EPD_BUSY_REN                  P2REN             /* EPD BUSY 上下拉使能寄存器。 */
#define EPD_BUSY_OUT                  P2OUT             /* EPD BUSY 输出寄存器。 */
#define EPD_BUSY_SEL                  P2SEL             /* EPD BUSY 功能选择寄存器。 */
#define EPD_BUSY_BIT                  BIT2              /* EPD BUSY 对应 P2.2。 */

#define EPD_RST_SEL                   P1SEL             /* EPD RST 功能选择寄存器。 */
#define EPD_RST_DIR                   P1DIR             /* EPD RST 方向寄存器。 */
#define EPD_RST_OUT                   P1OUT             /* EPD RST 输出寄存器。 */
#define EPD_RST_BIT                   BIT4              /* EPD RST 对应 P1.4。 */

#define EPD_DC_SEL                    P3SEL             /* EPD D/C 功能选择寄存器。 */
#define EPD_DC_DIR                    P3DIR             /* EPD D/C 方向寄存器。 */
#define EPD_DC_OUT                    P3OUT             /* EPD D/C 输出寄存器。 */
#define EPD_DC_BIT                    BIT4              /* EPD D/C 对应 P3.4。 */

#define EPD_CS_SEL                    P3SEL             /* EPD CS 功能选择寄存器。 */
#define EPD_CS_DIR                    P3DIR             /* EPD CS 方向寄存器。 */
#define EPD_CS_OUT                    P3OUT             /* EPD CS 输出寄存器。 */
#define EPD_CS_BIT                    BIT2              /* EPD CS 对应 P3.2。 */

#define EPD_SDI_SEL                   P3SEL             /* EPD SDI/MOSI 功能选择寄存器。 */
#define EPD_SDI_DIR                   P3DIR             /* EPD SDI/MOSI 方向寄存器。 */
#define EPD_SDI_OUT                   P3OUT             /* EPD SDI/MOSI 输出寄存器。 */
#define EPD_SDI_BIT                   BIT3              /* EPD SDI/MOSI 对应 P3.3。 */

#define EPD_CLK_SEL                   P2SEL             /* EPD SPI CLK 功能选择寄存器。 */
#define EPD_CLK_DIR                   P2DIR             /* EPD SPI CLK 方向寄存器。 */
#define EPD_CLK_OUT                   P2OUT             /* EPD SPI CLK 输出寄存器。 */
#define EPD_CLK_BIT                   BIT7              /* EPD SPI CLK 对应 P2.7。 */

/* 蜂鸣器：P3.6 输出方波，Pocket Kit 需要按文档连接跳线。 */
#define BUZZER_DIR                    P3DIR             /* 蜂鸣器引脚方向寄存器。 */
#define BUZZER_OUT                    P3OUT             /* 蜂鸣器引脚输出寄存器。 */
#define BUZZER_REN                    P3REN             /* 蜂鸣器引脚上下拉使能寄存器。 */
#define BUZZER_SEL                    P3SEL             /* 蜂鸣器引脚功能选择寄存器。 */
#define BUZZER_BIT                    BIT6              /* 蜂鸣器驱动信号对应 P3.6。 */
#define BUZZER_FREQ_HZ                5000u             /* 无源蜂鸣器驱动方波频率，单位 Hz。 */
#define BUZZER_HALF_PERIOD_TICKS      ((uint16_t)(SMCLK_HZ / (BUZZER_FREQ_HZ * 2u))) /* 蜂鸣器半周期 SMCLK 计数。 */

#endif
