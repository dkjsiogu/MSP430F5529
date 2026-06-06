#include "epd_panel_msp430.h"

#include "board.h"
#include "display_config.h"
#include "platform_config.h"

static const uint8_t epd_lut[29] = {
    0x50, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x11, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x0F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t epd_partial_lut[30] = {
    0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t epd_alt_lut[90] = {
    0x82, 0x00, 0x00, 0x00, 0xAA, 0x00, 0x00, 0x00,
    0xAA, 0xAA, 0x00, 0x00, 0xAA, 0xAA, 0xAA, 0x00,
    0x55, 0xAA, 0xAA, 0x00, 0x55, 0x55, 0x55, 0x55,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
    0xAA, 0xAA, 0xAA, 0xAA, 0x15, 0x15, 0x15, 0x15,
    0x05, 0x05, 0x05, 0x05, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x41, 0x45, 0xF1, 0xFF, 0x5F, 0x55, 0x01, 0x00,
    0x00, 0x00
};

/* 将墨水屏 SPI 控制线恢复到空闲电平。 */
static void epd_panel_bus_idle(void)
{
    EPD_CS_OUT |= EPD_CS_BIT;
    EPD_CLK_OUT &= (uint8_t)~EPD_CLK_BIT;
    EPD_SDI_OUT &= (uint8_t)~EPD_SDI_BIT;
    EPD_DC_OUT |= EPD_DC_BIT;
}

/* 初始化墨水屏使用的 GPIO 和软件 SPI 引脚方向。 */
static void epd_panel_spi_init(void)
{
    EPD_BUSY_SEL &= (uint8_t)~EPD_BUSY_BIT;
    EPD_RST_SEL &= (uint8_t)~EPD_RST_BIT;
    EPD_DC_SEL &= (uint8_t)~EPD_DC_BIT;
    EPD_CS_SEL &= (uint8_t)~EPD_CS_BIT;
    EPD_SDI_SEL &= (uint8_t)~EPD_SDI_BIT;
    EPD_CLK_SEL &= (uint8_t)~EPD_CLK_BIT;

    EPD_RST_OUT |= EPD_RST_BIT;
    epd_panel_bus_idle();

    EPD_BUSY_DIR &= (uint8_t)~EPD_BUSY_BIT;
    EPD_RST_DIR |= EPD_RST_BIT;
    EPD_DC_DIR |= EPD_DC_BIT;
    EPD_CS_DIR |= EPD_CS_BIT;
    EPD_SDI_DIR |= EPD_SDI_BIT;
    EPD_CLK_DIR |= EPD_CLK_BIT;

    EPD_RST_OUT |= EPD_RST_BIT;
    epd_panel_bus_idle();
}

/* 通过软件 SPI 向墨水屏发送一个字节。 */
static void epd_panel_spi_write(uint8_t value)
{
    uint8_t bit;

    for (bit = 0; bit < 8u; bit++) {
        if (value & 0x80u) {
            EPD_SDI_OUT |= EPD_SDI_BIT;
        } else {
            EPD_SDI_OUT &= (uint8_t)~EPD_SDI_BIT;
        }
        EPD_CLK_OUT |= EPD_CLK_BIT;
        EPD_CLK_OUT &= (uint8_t)~EPD_CLK_BIT;
        value = (uint8_t)(value << 1);
    }
}

/* 控制墨水屏片选信号，selected 为 1 时选中屏幕。 */
static void epd_panel_select(uint8_t selected)
{
    if (selected) {
        EPD_CS_OUT &= (uint8_t)~EPD_CS_BIT;
    } else {
        EPD_CS_OUT |= EPD_CS_BIT;
    }
}

/* 向墨水屏控制器发送一个命令字节。 */
static void epd_panel_cmd(uint8_t cmd)
{
    epd_panel_select(0);
    epd_panel_select(1);
    EPD_CLK_OUT &= (uint8_t)~EPD_CLK_BIT;
    EPD_DC_OUT &= (uint8_t)~EPD_DC_BIT;
    epd_panel_spi_write(cmd);
    epd_panel_select(0);
}

/* 向墨水屏控制器发送一个数据字节。 */
static void epd_panel_data(uint8_t data)
{
    epd_panel_select(0);
    epd_panel_select(1);
    EPD_CLK_OUT &= (uint8_t)~EPD_CLK_BIT;
    EPD_DC_OUT |= EPD_DC_BIT;
    epd_panel_spi_write(data);
    epd_panel_select(0);
}

/* 开始连续数据写入，减少每个字节反复切片选的开销。 */
static void epd_panel_data_stream_start(void)
{
    epd_panel_select(0);
    epd_panel_select(1);
    EPD_CLK_OUT &= (uint8_t)~EPD_CLK_BIT;
    EPD_DC_OUT |= EPD_DC_BIT;
}

/* 连续数据写入阶段发送一个字节。 */
static void epd_panel_data_stream_write(uint8_t data)
{
    epd_panel_spi_write(data);
}

/* 结束连续数据写入。 */
static void epd_panel_data_stream_end(void)
{
    epd_panel_select(0);
}

/* 等待 BUSY 释放，超时返回 0，避免屏幕异常时卡死任务。 */
static uint8_t epd_panel_wait_busy(uint16_t timeout_ms)
{
    while ((EPD_BUSY_IN & EPD_BUSY_BIT) && timeout_ms > 0) {
        delay_ms(1);
        timeout_ms--;
    }
    if (EPD_BUSY_IN & EPD_BUSY_BIT) {
        return 0;
    }
    delay_ms(20);
    return 1;
}

/* 等待一次刷新从 BUSY 拉高到释放，未检测到 BUSY 时使用兜底等待。 */
static uint8_t epd_panel_wait_update_done(uint16_t post_update_ms, uint16_t no_busy_wait_ms)
{
    uint16_t timeout_ms;
    uint8_t saw_busy;

    saw_busy = 0;
    timeout_ms = EPD_BUSY_START_TIMEOUT_MS;
    while (timeout_ms > 0) {
        if (EPD_BUSY_IN & EPD_BUSY_BIT) {
            saw_busy = 1;
            break;
        }
        delay_ms(1);
        timeout_ms--;
    }

    if (saw_busy) {
        if (!epd_panel_wait_busy(EPD_BUSY_TIMEOUT_MS)) {
            return 0;
        }
    } else {
        delay_ms(no_busy_wait_ms);
    }

    delay_ms(post_update_ms);
    return saw_busy;
}

/* 对墨水屏执行硬复位。 */
static void epd_panel_reset(void)
{
    epd_panel_bus_idle();
    EPD_RST_OUT |= EPD_RST_BIT;
    delay_ms(EPD_RESET_PRE_MS);
    EPD_RST_OUT &= (uint8_t)~EPD_RST_BIT;
    delay_ms(EPD_RESET_LOW_MS);
    EPD_RST_OUT |= EPD_RST_BIT;
    delay_ms(EPD_RESET_HIGH_MS);
    epd_panel_bus_idle();
}

/* 对控制器执行软件复位，并等待 BUSY 释放。 */
static uint8_t epd_panel_soft_reset(void)
{
    uint16_t timeout_ms;
    uint8_t saw_busy;

    epd_panel_cmd(0x12);
    delay_ms(1);
    saw_busy = 0;
    timeout_ms = EPD_BUSY_START_TIMEOUT_MS;
    while (timeout_ms > 0) {
        if (EPD_BUSY_IN & EPD_BUSY_BIT) {
            saw_busy = 1;
            break;
        }
        delay_ms(1);
        timeout_ms--;
    }
    if (saw_busy) {
        return epd_panel_wait_busy(EPD_BUSY_TIMEOUT_MS);
    }
    delay_ms(100);
    return 1;
}

/* 从未知上电状态恢复面板控制器，兼容烧录后和冷启动两种路径。 */
static void epd_panel_recover_from_unknown_state(void)
{
    epd_panel_spi_init();
    delay_ms(EPD_STARTUP_SETTLE_MS);
    epd_panel_reset();
    (void)epd_panel_wait_busy(EPD_BUSY_TIMEOUT_MS);
    (void)epd_panel_soft_reset();
    (void)epd_panel_wait_busy(EPD_BUSY_TIMEOUT_MS);
}

/* 写入 SSD1673 全刷 LUT。 */
static void epd_panel_write_lut(void)
{
    uint8_t i;

    epd_panel_cmd(0x32);
    epd_panel_data_stream_start();
    for (i = 0; i < sizeof(epd_lut); i++) {
        epd_panel_data_stream_write(epd_lut[i]);
    }
    epd_panel_data_stream_end();
}

/* 写入 SSD1673 局刷 LUT。 */
static void epd_panel_write_partial_lut(void)
{
    uint8_t i;

    epd_panel_cmd(0x32);
    epd_panel_data_stream_start();
    for (i = 0; i < sizeof(epd_partial_lut); i++) {
        epd_panel_data_stream_write(epd_partial_lut[i]);
    }
    epd_panel_data_stream_end();
}

/* 写入备用控制器 LUT。 */
static void epd_panel_alt_write_lut(void)
{
    uint8_t i;

    epd_panel_cmd(0x32);
    epd_panel_data_stream_start();
    for (i = 0; i < sizeof(epd_alt_lut); i++) {
        epd_panel_data_stream_write(epd_alt_lut[i]);
    }
    epd_panel_data_stream_end();
}

/* 配置 SSD1673 图像 RAM 区域。 */
static void epd_panel_set_ram_area(void)
{
    epd_panel_cmd(0x11);
    epd_panel_data(0x01);
    epd_panel_cmd(0x44);
    epd_panel_data(0x00);
    epd_panel_data(0x0F);
    epd_panel_cmd(0x45);
    epd_panel_data(0xF9);
    epd_panel_data(0x00);
}

/* 配置 SSD1673 图像 RAM 光标。 */
static void epd_panel_set_ram_cursor(void)
{
    epd_panel_cmd(0x4E);
    epd_panel_data(0x00);
    epd_panel_cmd(0x4F);
    epd_panel_data(0xF9);
}

/* 配置备用控制器图像 RAM 区域和光标。 */
static void epd_panel_alt_set_ram_area(void)
{
    epd_panel_cmd(0x11);
    epd_panel_data(0x03);
    epd_panel_cmd(0x44);
    epd_panel_data(0x00);
    epd_panel_data(0x11);
    epd_panel_cmd(0x45);
    epd_panel_data(0x00);
    epd_panel_data(0xAB);
    epd_panel_cmd(0x4E);
    epd_panel_data(0x00);
    epd_panel_cmd(0x4F);
    epd_panel_data(0xAB);
}

/* 触发一次控制字指定的 SSD1673 刷新。 */
static uint8_t epd_panel_update_with_ctrl(uint8_t ctrl, uint16_t post_update_ms, uint16_t no_busy_wait_ms)
{
    uint8_t saw_busy;

    epd_panel_cmd(0x22);
    epd_panel_data(ctrl);
    epd_panel_cmd(0x20);
    delay_ms(1);
    saw_busy = epd_panel_wait_update_done(post_update_ms, no_busy_wait_ms);
    return saw_busy;
}

/* 对 SSD1673 执行一次全屏刷新。 */
static uint8_t epd_panel_update_full(void)
{
    return epd_panel_update_with_ctrl(EPD_UPDATE_CTRL_FULL, EPD_FULL_POST_UPDATE_MS, EPD_FULL_POST_UPDATE_MS);
}

/* 对备用驱动执行一次局部刷新，并让控制器回到待机刷新状态。 */
static uint8_t epd_panel_alt_update(void)
{
    uint8_t saw_busy;

    epd_panel_cmd(0x21);
    epd_panel_data(0x83);
    epd_panel_cmd(0x22);
    epd_panel_data(0xC4);
    epd_panel_cmd(0x20);
    delay_ms(100);
    saw_busy = epd_panel_wait_update_done(EPD_PARTIAL_POST_UPDATE_MS, EPD_NO_BUSY_FALLBACK_MS);

    epd_panel_cmd(0x22);
    epd_panel_data(0x03);
    epd_panel_cmd(0x20);
    delay_ms(20);
    return saw_busy;
}

/* 准备向 SSD1673 新图像 RAM 写入数据。 */
static void epd_panel_write_new_ram_start(void)
{
    epd_panel_set_ram_area();
    epd_panel_set_ram_cursor();
    delay_ms(1);
    epd_panel_cmd(0x24);
    delay_ms(1);
}

/* 用同一个字节填充 SSD1673 新图像 RAM。 */
static void epd_panel_write_new_ram_fill_only(uint8_t value)
{
    uint16_t col;
    uint16_t row;

    epd_panel_write_new_ram_start();
    epd_panel_data_stream_start();
    for (col = 0; col < EPD_RAM_H; col++) {
        for (row = 0; row < EPD_RAM_W_BYTES; row++) {
            epd_panel_data_stream_write(value);
        }
    }
    epd_panel_data_stream_end();
}

/* 将帧缓冲内容写入 SSD1673 新图像 RAM，但暂不触发刷新。 */
static void epd_panel_write_new_ram_buffer_only(const uint8_t *buf)
{
    uint16_t i;

    epd_panel_write_new_ram_start();
    epd_panel_data_stream_start();
    for (i = 0; i < EPD_BUF_SIZE; i++) {
        epd_panel_data_stream_write(buf[i]);
    }
    epd_panel_data_stream_end();
}

/* 将帧缓冲内容写入备用驱动图像 RAM，但暂不触发刷新。 */
static void epd_panel_alt_write_ram_buffer_only(const uint8_t *buf)
{
    uint16_t i;

    epd_panel_alt_set_ram_area();
    epd_panel_cmd(0x24);
    epd_panel_data_stream_start();
    for (i = 0; i < EPD_ALT_BUF_SIZE; i++) {
        epd_panel_data_stream_write(buf[i]);
    }
    epd_panel_data_stream_end();
}

void epd_panel_ssd_init_controller_and_clear(void)
{
    EPD_BUSY_REN &= (uint8_t)~EPD_BUSY_BIT;

    epd_panel_recover_from_unknown_state();

    epd_panel_cmd(0x01);
    epd_panel_data(0xF9);
    epd_panel_data(0x00);
    epd_panel_cmd(0x3A);
    epd_panel_data(0x06);
    epd_panel_cmd(0x3B);
    epd_panel_data(0x0B);
    epd_panel_cmd(0x3C);
    epd_panel_data(0x33);
    epd_panel_set_ram_area();

    epd_panel_cmd(0x2C);
    epd_panel_data(0x4B);
    epd_panel_write_lut();

    epd_panel_cmd(0x21);
    epd_panel_data(0x83);
    epd_panel_write_new_ram_fill_only(0xFF);
    (void)epd_panel_update_full();
    epd_panel_cmd(0x21);
    epd_panel_data(0x03);
    epd_panel_cmd(0x3C);
    epd_panel_data(0x73);
    epd_panel_write_partial_lut();
}

void epd_panel_alt_init(void)
{
    EPD_BUSY_REN &= (uint8_t)~EPD_BUSY_BIT;

    epd_panel_recover_from_unknown_state();

    epd_panel_cmd(0x10);
    epd_panel_data(0x00);
    epd_panel_cmd(0x11);
    epd_panel_data(0x03);
    epd_panel_cmd(0x44);
    epd_panel_data(0x00);
    epd_panel_data(0x11);
    epd_panel_cmd(0x45);
    epd_panel_data(0x00);
    epd_panel_data(0xAB);
    epd_panel_cmd(0x4E);
    epd_panel_data(0x00);
    epd_panel_cmd(0x4F);
    epd_panel_data(0xAB);
    epd_panel_cmd(0x21);
    epd_panel_data(0x03);
    epd_panel_cmd(0xF0);
    epd_panel_data(0x1F);
    epd_panel_cmd(0x2C);
    epd_panel_data(0xA0);
    epd_panel_cmd(0x3C);
    epd_panel_data(0x63);
    epd_panel_cmd(0x22);
    epd_panel_data(0xC4);
    epd_panel_alt_write_lut();
}

uint8_t epd_panel_ssd_flush_partial(const uint8_t *buf)
{
    uint8_t busy_seen;

    epd_panel_write_partial_lut();
    epd_panel_cmd(0x21);
    epd_panel_data(0x03);
    epd_panel_cmd(0x3C);
    epd_panel_data(0x73);
    epd_panel_write_new_ram_buffer_only(buf);
    busy_seen = epd_panel_update_with_ctrl(EPD_UPDATE_CTRL_PARTIAL,
                                           EPD_PARTIAL_POST_UPDATE_MS,
                                           EPD_NO_BUSY_FALLBACK_MS);
    epd_panel_write_new_ram_buffer_only(buf);
    return busy_seen;
}

uint8_t epd_panel_alt_flush(const uint8_t *buf)
{
    epd_panel_alt_write_ram_buffer_only(buf);
    return epd_panel_alt_update();
}
