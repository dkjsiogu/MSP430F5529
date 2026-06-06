/*
 * HAL_SDCard.c
 * Pocket Kit SD 卡 SPI 硬件适配层：配置 UCB1 和 P4.0 片选，
 * 供 FatFs 的 mmc.c 完成 SDHC/FAT32 扇区读写。
 */
#include "HAL_SDCard.h"
#include "platform_config.h"

#define SD_SPI_SIMO_BIT          BIT1      /* UCB1SIMO 对应 P4.1。 */
#define SD_SPI_SOMI_BIT          BIT2      /* UCB1SOMI 对应 P4.2。 */
#define SD_SPI_CLK_BIT           BIT3      /* UCB1CLK 对应 P4.3。 */
#define SD_CS_BIT                BIT0      /* SD 卡片选信号对应 P4.0。 */
#define SD_SPI_SEL               P4SEL     /* SD SPI 引脚功能选择寄存器。 */
#define SD_SPI_DIR               P4DIR     /* SD SPI 引脚方向寄存器。 */
#define SD_SPI_OUT               P4OUT     /* SD SPI 引脚输出寄存器。 */
#define SD_SPI_REN               P4REN     /* SD SPI 引脚上下拉使能寄存器。 */
#define SD_CS_SEL                P4SEL     /* SD CS 引脚功能选择寄存器。 */
#define SD_CS_DIR                P4DIR     /* SD CS 引脚方向寄存器。 */
#define SD_CS_OUT                P4OUT     /* SD CS 引脚输出寄存器。 */
#define SD_CS_REN                P4REN     /* SD CS 引脚上下拉使能寄存器。 */
#define SD_INIT_DIVIDER          ((uint16_t)(SMCLK_HZ / 400000UL)) /* 初始化阶段 SPI 分频，约 400kHz。 */
#define SD_FAST_DIVIDER          4u        /* 正常读写阶段 SPI 分频，16MHz/4=4MHz。 */

#define SD_SPI_WAIT_LIMIT        60000u

static uint8_t SDCard_waitFlag(uint8_t flag)
{
    uint16_t guard;

    guard = SD_SPI_WAIT_LIMIT;
    while (!(UCB1IFG & flag) && guard > 0u) {
        guard--;
    }
    return (uint8_t)((UCB1IFG & flag) != 0u);
}

unsigned char SDCard_init(void)
{
    SD_SPI_DIR |= SD_SPI_CLK_BIT | SD_SPI_SIMO_BIT;
    SD_SPI_REN |= SD_SPI_SOMI_BIT;
    SD_SPI_OUT |= SD_SPI_SOMI_BIT;
    SD_SPI_SEL |= SD_SPI_CLK_BIT | SD_SPI_SOMI_BIT | SD_SPI_SIMO_BIT;

    SD_CS_SEL &= (uint8_t)~SD_CS_BIT;
    SD_CS_REN |= SD_CS_BIT;
    SD_CS_DIR |= SD_CS_BIT;
    SD_CS_OUT |= SD_CS_BIT;

    UCB1CTL1 |= UCSWRST;
    UCB1CTL0 = UCCKPL | UCMSB | UCMST | UCMODE_0 | UCSYNC;
    UCB1CTL1 = UCSWRST | UCSSEL_2;
    UCB1BR0 = (uint8_t)(SD_INIT_DIVIDER & 0xFFu);
    UCB1BR1 = (uint8_t)(SD_INIT_DIVIDER >> 8);
    UCB1CTL1 &= (uint8_t)~UCSWRST;
    UCB1IFG &= (uint8_t)~UCRXIFG;
    return 0;
}

void SDCard_fastMode(void)
{
    UCB1CTL1 |= UCSWRST;
    UCB1BR0 = SD_FAST_DIVIDER;
    UCB1BR1 = 0;
    UCB1CTL1 &= (uint8_t)~UCSWRST;
}

uint8_t SDCard_readFrame(uint8_t *pBuffer, uint16_t size)
{
    while (size--) {
        if (!SDCard_waitFlag(UCTXIFG)) {
            return 0;
        }
        UCB1TXBUF = 0xFFu;
        if (!SDCard_waitFlag(UCRXIFG)) {
            return 0;
        }
        *pBuffer++ = UCB1RXBUF;
    }
    return 1;
}

uint8_t SDCard_sendFrame(const uint8_t *pBuffer, uint16_t size)
{
    while (size--) {
        if (!SDCard_waitFlag(UCTXIFG)) {
            return 0;
        }
        UCB1TXBUF = *pBuffer++;
        if (!SDCard_waitFlag(UCRXIFG)) {
            return 0;
        }
        (void)UCB1RXBUF;
    }
    return 1;
}

void SDCard_setCSHigh(void)
{
    SD_CS_OUT |= SD_CS_BIT;
}

void SDCard_setCSLow(void)
{
    SD_CS_OUT &= (uint8_t)~SD_CS_BIT;
}
