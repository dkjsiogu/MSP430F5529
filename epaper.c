#include "epaper.h"
#include "app_state.h"
#include "board.h"
#include "flash_log.h"
#include "format.h"
static const uint8_t font5x7[96][5] = {
    {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5F,0x00,0x00},
    {0x00,0x07,0x00,0x07,0x00}, {0x14,0x7F,0x14,0x7F,0x14},
    {0x24,0x2A,0x7F,0x2A,0x12}, {0x23,0x13,0x08,0x64,0x62},
    {0x36,0x49,0x55,0x22,0x50}, {0x00,0x05,0x03,0x00,0x00},
    {0x00,0x1C,0x22,0x41,0x00}, {0x00,0x41,0x22,0x1C,0x00},
    {0x14,0x08,0x3E,0x08,0x14}, {0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08},
    {0x00,0x60,0x60,0x00,0x00}, {0x20,0x10,0x08,0x04,0x02},
    {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E},
    {0x00,0x36,0x36,0x00,0x00}, {0x00,0x56,0x36,0x00,0x00},
    {0x08,0x14,0x22,0x41,0x00}, {0x14,0x14,0x14,0x14,0x14},
    {0x00,0x41,0x22,0x14,0x08}, {0x02,0x01,0x51,0x09,0x06},
    {0x32,0x49,0x79,0x41,0x3E}, {0x7E,0x11,0x11,0x11,0x7E},
    {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C}, {0x7F,0x49,0x49,0x49,0x41},
    {0x7F,0x09,0x09,0x09,0x01}, {0x3E,0x41,0x49,0x49,0x7A},
    {0x7F,0x08,0x08,0x08,0x7F}, {0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41},
    {0x7F,0x40,0x40,0x40,0x40}, {0x7F,0x02,0x0C,0x02,0x7F},
    {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06}, {0x3E,0x41,0x51,0x21,0x5E},
    {0x7F,0x09,0x19,0x29,0x46}, {0x46,0x49,0x49,0x49,0x31},
    {0x01,0x01,0x7F,0x01,0x01}, {0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F}, {0x3F,0x40,0x38,0x40,0x3F},
    {0x63,0x14,0x08,0x14,0x63}, {0x07,0x08,0x70,0x08,0x07},
    {0x61,0x51,0x49,0x45,0x43}, {0x00,0x7F,0x41,0x41,0x00},
    {0x02,0x04,0x08,0x10,0x20}, {0x00,0x41,0x41,0x7F,0x00},
    {0x04,0x02,0x01,0x02,0x04}, {0x40,0x40,0x40,0x40,0x40},
    {0x00,0x01,0x02,0x04,0x00}, {0x20,0x54,0x54,0x54,0x78},
    {0x7F,0x48,0x44,0x44,0x38}, {0x38,0x44,0x44,0x44,0x20},
    {0x38,0x44,0x44,0x48,0x7F}, {0x38,0x54,0x54,0x54,0x18},
    {0x08,0x7E,0x09,0x01,0x02}, {0x0C,0x52,0x52,0x52,0x3E},
    {0x7F,0x08,0x04,0x04,0x78}, {0x00,0x44,0x7D,0x40,0x00},
    {0x20,0x40,0x44,0x3D,0x00}, {0x7F,0x10,0x28,0x44,0x00},
    {0x00,0x41,0x7F,0x40,0x00}, {0x7C,0x04,0x18,0x04,0x78},
    {0x7C,0x08,0x04,0x04,0x78}, {0x38,0x44,0x44,0x44,0x38},
    {0x7C,0x14,0x14,0x14,0x08}, {0x08,0x14,0x14,0x18,0x7C},
    {0x7C,0x08,0x04,0x04,0x08}, {0x48,0x54,0x54,0x54,0x20},
    {0x04,0x3F,0x44,0x40,0x20}, {0x3C,0x40,0x40,0x20,0x7C},
    {0x1C,0x20,0x40,0x20,0x1C}, {0x3C,0x40,0x30,0x40,0x3C},
    {0x44,0x28,0x10,0x28,0x44}, {0x0C,0x50,0x50,0x50,0x3C},
    {0x44,0x64,0x54,0x4C,0x44}, {0x00,0x08,0x36,0x41,0x00},
    {0x00,0x00,0x7F,0x00,0x00}, {0x00,0x41,0x36,0x08,0x00},
    {0x10,0x08,0x08,0x10,0x08}, {0x00,0x06,0x09,0x09,0x06}
};
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

#define EPD_VIEW_CURRENT            0u
#define EPD_VIEW_HISTORY            1u
#define EPD_VIEW_SETTINGS           2u

static uint8_t g_epd_auto_update = 1;
static uint8_t g_epd_driver = EPD_DRIVER_SSD1673;
static uint8_t g_epd_render_view = EPD_VIEW_CURRENT;
static uint8_t g_epd_render_dirty = 1;
static uint8_t g_epd_has_target_sample = 0;
static uint8_t g_epd_settings_selected = 0;
static uint8_t g_epd_settings_editing = 0;
static uint16_t g_epd_target_history_start = 0;
static TempSample g_epd_target_sample;
static uint8_t epd_buf[EPD_BUF_SIZE];
static uint8_t g_anim_frame = 0;

static uint8_t epd_next_anim_frame(void)
{
    uint8_t frame;

    frame = g_anim_frame;
    g_anim_frame = (uint8_t)((g_anim_frame + 1u) & 0x07u);
    return frame;
}

static void epd_bus_idle(void)
{
    EPD_CS_OUT |= EPD_CS_BIT;
    EPD_CLK_OUT &= ~EPD_CLK_BIT;
    EPD_SDI_OUT &= ~EPD_SDI_BIT;
    EPD_DC_OUT |= EPD_DC_BIT;
}

static void spi0_init(void)
{
    EPD_BUSY_SEL &= ~EPD_BUSY_BIT;
    EPD_RST_SEL &= ~EPD_RST_BIT;
    EPD_DC_SEL &= ~EPD_DC_BIT;
    EPD_CS_SEL &= ~EPD_CS_BIT;
    EPD_SDI_SEL &= ~EPD_SDI_BIT;
    EPD_CLK_SEL &= ~EPD_CLK_BIT;

    EPD_RST_OUT |= EPD_RST_BIT;
    epd_bus_idle();

    EPD_BUSY_DIR &= ~EPD_BUSY_BIT;
    EPD_RST_DIR |= EPD_RST_BIT;
    EPD_DC_DIR |= EPD_DC_BIT;
    EPD_CS_DIR |= EPD_CS_BIT;
    EPD_SDI_DIR |= EPD_SDI_BIT;
    EPD_CLK_DIR |= EPD_CLK_BIT;

    EPD_RST_OUT |= EPD_RST_BIT;
    epd_bus_idle();
}

static void epd_spi_delay(void)
{
    __delay_cycles(8);
}

static void spi0_write(uint8_t value)
{
    uint8_t bit;

    for (bit = 0; bit < 8u; bit++) {
        if (value & 0x80u) {
            EPD_SDI_OUT |= EPD_SDI_BIT;
        } else {
            EPD_SDI_OUT &= ~EPD_SDI_BIT;
        }
        epd_spi_delay();
        EPD_CLK_OUT |= EPD_CLK_BIT;
        epd_spi_delay();
        EPD_CLK_OUT &= ~EPD_CLK_BIT;
        value = (uint8_t)(value << 1);
        epd_spi_delay();
    }
}

static void epd_select(uint8_t selected)
{
    if (selected) {
        EPD_CS_OUT &= ~EPD_CS_BIT;
    } else {
        EPD_CS_OUT |= EPD_CS_BIT;
    }
}

static void epd_cmd(uint8_t cmd)
{
    epd_select(0);
    epd_select(1);
    EPD_CLK_OUT &= ~EPD_CLK_BIT;
    EPD_DC_OUT &= ~EPD_DC_BIT;
    spi0_write(cmd);
    epd_select(0);
}

static void epd_data(uint8_t data)
{
    epd_select(0);
    epd_select(1);
    EPD_CLK_OUT &= ~EPD_CLK_BIT;
    EPD_DC_OUT |= EPD_DC_BIT;
    spi0_write(data);
    epd_select(0);
}

static uint8_t epd_wait_busy(uint16_t timeout_ms)
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

static uint8_t epd_wait_update_done(uint16_t post_update_ms, uint16_t no_busy_wait_ms)
{
    uint16_t timeout_ms;
    uint8_t saw_busy;

    timeout_ms = EPD_BUSY_START_TIMEOUT_MS;
    saw_busy = 0;
    while (timeout_ms > 0) {
        if (EPD_BUSY_IN & EPD_BUSY_BIT) {
            saw_busy = 1;
            break;
        }
        delay_ms(1);
        timeout_ms--;
    }

    if (saw_busy) {
        if (!epd_wait_busy(EPD_BUSY_TIMEOUT_MS)) {
            return 0;
        }
    } else {
        delay_ms(no_busy_wait_ms);
    }

    delay_ms(post_update_ms);
    return saw_busy;
}

static void epd_reset(void)
{
    epd_bus_idle();
    EPD_RST_OUT |= EPD_RST_BIT;
    delay_ms(EPD_RESET_PRE_MS);
    EPD_RST_OUT &= ~EPD_RST_BIT;
    delay_ms(EPD_RESET_LOW_MS);
    EPD_RST_OUT |= EPD_RST_BIT;
    delay_ms(EPD_RESET_HIGH_MS);
    epd_bus_idle();
}

static uint8_t epd_soft_reset(void)
{
    uint16_t timeout_ms;
    uint8_t saw_busy;

    epd_cmd(0x12);
    delay_ms(1);

    timeout_ms = EPD_BUSY_START_TIMEOUT_MS;
    saw_busy = 0;
    while (timeout_ms > 0) {
        if (EPD_BUSY_IN & EPD_BUSY_BIT) {
            saw_busy = 1;
            break;
        }
        delay_ms(1);
        timeout_ms--;
    }

    if (saw_busy) {
        return epd_wait_busy(EPD_BUSY_TIMEOUT_MS);
    }

    delay_ms(100);
    return 1;
}

static void epd_recover_from_unknown_state(void)
{
    spi0_init();
    delay_ms(EPD_STARTUP_SETTLE_MS);
    epd_reset();
    (void)epd_wait_busy(EPD_BUSY_TIMEOUT_MS);
    (void)epd_soft_reset();
    (void)epd_wait_busy(EPD_BUSY_TIMEOUT_MS);
}

static void epd_write_lut(void)
{
    uint8_t i;

    epd_cmd(0x32);
    for (i = 0; i < sizeof(epd_lut); i++) {
        epd_data(epd_lut[i]);
    }
}

static void epd_write_partial_lut(void)
{
    uint8_t i;

    epd_cmd(0x32);
    for (i = 0; i < sizeof(epd_partial_lut); i++) {
        epd_data(epd_partial_lut[i]);
    }
}

static void epd_alt_write_lut(void)
{
    uint8_t i;

    epd_cmd(0x32);
    for (i = 0; i < sizeof(epd_alt_lut); i++) {
        epd_data(epd_alt_lut[i]);
    }
}

static void epd_set_ram_area(void)
{
    epd_cmd(0x11);
    epd_data(0x01);
    epd_cmd(0x44);
    epd_data(0x00);
    epd_data(0x0F);
    epd_cmd(0x45);
    epd_data(0xF9);
    epd_data(0x00);
}

static void epd_set_ram_cursor(void)
{
    epd_cmd(0x4E);
    epd_data(0x00);
    epd_cmd(0x4F);
    epd_data(0xF9);
}

static void epd_alt_set_ram_area(void)
{
    epd_cmd(0x11);
    epd_data(0x03);
    epd_cmd(0x44);
    epd_data(0x00);
    epd_data(0x11);
    epd_cmd(0x45);
    epd_data(0x00);
    epd_data(0xAB);
    epd_cmd(0x4E);
    epd_data(0x00);
    epd_cmd(0x4F);
    epd_data(0xAB);
}

static uint8_t epd_update_with_ctrl(uint8_t ctrl, uint16_t post_update_ms, uint16_t no_busy_wait_ms)
{
    uint8_t saw_busy;

    epd_cmd(0x22);
    epd_data(ctrl);
    epd_cmd(0x20);
    delay_ms(1);
    saw_busy = epd_wait_update_done(post_update_ms, no_busy_wait_ms);
    return saw_busy;
}

static uint8_t epd_update(void)
{
    return epd_update_with_ctrl(EPD_UPDATE_CTRL_FULL, EPD_FULL_POST_UPDATE_MS, EPD_FULL_POST_UPDATE_MS);
}

static uint8_t epd_alt_update(void)
{
    uint8_t saw_busy;

    epd_cmd(0x21);
    epd_data(0x83);
    epd_cmd(0x22);
    epd_data(0xC4);
    epd_cmd(0x20);
    delay_ms(100);
    saw_busy = epd_wait_update_done(EPD_PARTIAL_POST_UPDATE_MS, EPD_NO_BUSY_FALLBACK_MS);

    epd_cmd(0x22);
    epd_data(0x03);
    epd_cmd(0x20);
    delay_ms(20);
    return saw_busy;
}

static void epd_write_new_ram_start(void)
{
    epd_set_ram_area();
    epd_set_ram_cursor();
    delay_ms(5);
    epd_cmd(0x24);
    delay_ms(5);
}

static void epd_write_new_ram_fill_only(uint8_t value)
{
    uint16_t col;
    uint16_t row;

    epd_write_new_ram_start();
    for (col = 0; col < EPD_RAM_H; col++) {
        for (row = 0; row < EPD_RAM_W_BYTES; row++) {
            epd_data(value);
        }
    }
}

static void epd_write_new_ram_buffer_only(const uint8_t *buf)
{
    uint16_t i;

    epd_write_new_ram_start();
    for (i = 0; i < EPD_BUF_SIZE; i++) {
        epd_data(buf[i]);
    }
}

static void epd_alt_write_ram_buffer_only(const uint8_t *buf)
{
    uint16_t i;

    epd_alt_set_ram_area();
    epd_cmd(0x24);
    for (i = 0; i < EPD_ALT_BUF_SIZE; i++) {
        epd_data(buf[i]);
    }
}

static uint8_t epd_write_buffer_to_screen_partial(const uint8_t *buf)
{
    uint8_t busy_seen;

    epd_write_partial_lut();
    epd_cmd(0x21);
    epd_data(0x03);
    epd_cmd(0x3C);
    epd_data(0x73);
    epd_write_new_ram_buffer_only(buf);
    busy_seen = epd_update_with_ctrl(EPD_UPDATE_CTRL_PARTIAL, EPD_PARTIAL_POST_UPDATE_MS, EPD_NO_BUSY_FALLBACK_MS);
    epd_write_new_ram_buffer_only(buf);
    return busy_seen;
}

static uint8_t epd_alt_write_buffer_to_screen(const uint8_t *buf)
{
    epd_alt_write_ram_buffer_only(buf);
    return epd_alt_update();
}

static void epd_ssd_init_controller_and_clear(void)
{
    EPD_BUSY_REN &= ~EPD_BUSY_BIT;

    epd_recover_from_unknown_state();

    epd_cmd(0x01);
    epd_data(0xF9);
    epd_data(0x00);
    epd_cmd(0x3A);
    epd_data(0x06);
    epd_cmd(0x3B);
    epd_data(0x0B);
    epd_cmd(0x3C);
    epd_data(0x33);
    epd_set_ram_area();

    epd_cmd(0x2C);
    epd_data(0x4B);
    epd_write_lut();

    epd_cmd(0x21);
    epd_data(0x83);
    epd_write_new_ram_fill_only(0xFF);
    epd_update();
    epd_cmd(0x21);
    epd_data(0x03);
    epd_cmd(0x3C);
    epd_data(0x73);
    epd_write_partial_lut();
}

void epd_init(void)
{
    epd_ssd_init_controller_and_clear();

    g_epd_has_target_sample = 0;
    g_epd_render_view = EPD_VIEW_CURRENT;
    g_epd_render_dirty = 1;
}

static void epd_alt_init(void)
{
    EPD_BUSY_REN &= ~EPD_BUSY_BIT;

    epd_recover_from_unknown_state();

    epd_cmd(0x10);
    epd_data(0x00);
    epd_cmd(0x11);
    epd_data(0x03);
    epd_cmd(0x44);
    epd_data(0x00);
    epd_data(0x11);
    epd_cmd(0x45);
    epd_data(0x00);
    epd_data(0xAB);
    epd_cmd(0x4E);
    epd_data(0x00);
    epd_cmd(0x4F);
    epd_data(0xAB);
    epd_cmd(0x21);
    epd_data(0x03);
    epd_cmd(0xF0);
    epd_data(0x1F);
    epd_cmd(0x2C);
    epd_data(0xA0);
    epd_cmd(0x3C);
    epd_data(0x63);
    epd_cmd(0x22);
    epd_data(0xC4);
    epd_alt_write_lut();
}

static void epd_clear_buffer(void)
{
    uint16_t i;

    for (i = 0; i < EPD_BUF_SIZE; i++) {
        epd_buf[i] = 0xFF;
    }
}

static void epd_pixel(uint16_t x, uint16_t y, uint8_t black)
{
    uint16_t ram_x;
    uint16_t ram_y;
    uint16_t index;
    uint8_t mask;

    if (x >= EPD_SCREEN_W || y >= EPD_SCREEN_H) {
        return;
    }

    ram_x = y;
    ram_y = x;
    index = (uint16_t)(ram_y * EPD_RAM_W_BYTES + (ram_x >> 3));
    mask = (uint8_t)(0x80u >> (ram_x & 7u));

    if (black) {
        epd_buf[index] &= (uint8_t)(~mask);
    } else {
        epd_buf[index] |= mask;
    }
}

static void epd_fill_rect_i(int16_t x, int16_t y, uint8_t w, uint8_t h)
{
    uint8_t dx;
    uint8_t dy;
    int16_t px;
    int16_t py;

    for (dy = 0; dy < h; dy++) {
        py = (int16_t)(y + dy);
        if (py < 0) {
            continue;
        }
        for (dx = 0; dx < w; dx++) {
            px = (int16_t)(x + dx);
            if (px >= 0) {
                epd_pixel((uint16_t)px, (uint16_t)py, 1);
            }
        }
    }
}

static void epd_draw_status_anim(uint16_t cx, uint16_t cy, uint8_t frame)
{
    static const int8_t tx[8] = {0, 9, 12, 9, 0, -9, -12, -9};
    static const int8_t ty[8] = {-12, -9, 0, 9, 12, 9, 0, -9};
    uint8_t prev;
    int16_t x;
    int16_t y;

    frame &= 0x07u;
    prev = (uint8_t)((frame + 7u) & 0x07u);
    x = (int16_t)cx;
    y = (int16_t)cy;

    epd_fill_rect_i((int16_t)(x - 15), (int16_t)(y - 15), 8u, 2u);
    epd_fill_rect_i((int16_t)(x - 15), (int16_t)(y - 15), 2u, 8u);
    epd_fill_rect_i((int16_t)(x + 8), (int16_t)(y - 15), 8u, 2u);
    epd_fill_rect_i((int16_t)(x + 14), (int16_t)(y - 15), 2u, 8u);
    epd_fill_rect_i((int16_t)(x - 15), (int16_t)(y + 14), 8u, 2u);
    epd_fill_rect_i((int16_t)(x - 15), (int16_t)(y + 8), 2u, 8u);
    epd_fill_rect_i((int16_t)(x + 8), (int16_t)(y + 14), 8u, 2u);
    epd_fill_rect_i((int16_t)(x + 14), (int16_t)(y + 8), 2u, 8u);

    epd_fill_rect_i((int16_t)(x - 2), (int16_t)(y - 2), 5u, 5u);
    epd_fill_rect_i((int16_t)(x - 7), y, 3u, 1u);
    epd_fill_rect_i((int16_t)(x + 5), y, 3u, 1u);
    epd_fill_rect_i(x, (int16_t)(y - 7), 1u, 3u);
    epd_fill_rect_i(x, (int16_t)(y + 5), 1u, 3u);

    epd_fill_rect_i((int16_t)(x + tx[prev] - 1), (int16_t)(y + ty[prev] - 1), 3u, 3u);

    switch (frame) {
    case 0:
        epd_fill_rect_i((int16_t)(x - 1), (int16_t)(y - 13), 3u, 11u);
        break;
    case 1:
        epd_fill_rect_i((int16_t)(x + 3), (int16_t)(y - 5), 3u, 3u);
        epd_fill_rect_i((int16_t)(x + 6), (int16_t)(y - 8), 3u, 3u);
        epd_fill_rect_i((int16_t)(x + 9), (int16_t)(y - 11), 3u, 3u);
        break;
    case 2:
        epd_fill_rect_i((int16_t)(x + 2), (int16_t)(y - 1), 11u, 3u);
        break;
    case 3:
        epd_fill_rect_i((int16_t)(x + 3), (int16_t)(y + 3), 3u, 3u);
        epd_fill_rect_i((int16_t)(x + 6), (int16_t)(y + 6), 3u, 3u);
        epd_fill_rect_i((int16_t)(x + 9), (int16_t)(y + 9), 3u, 3u);
        break;
    case 4:
        epd_fill_rect_i((int16_t)(x - 1), (int16_t)(y + 2), 3u, 11u);
        break;
    case 5:
        epd_fill_rect_i((int16_t)(x - 5), (int16_t)(y + 3), 3u, 3u);
        epd_fill_rect_i((int16_t)(x - 8), (int16_t)(y + 6), 3u, 3u);
        epd_fill_rect_i((int16_t)(x - 11), (int16_t)(y + 9), 3u, 3u);
        break;
    case 6:
        epd_fill_rect_i((int16_t)(x - 13), (int16_t)(y - 1), 11u, 3u);
        break;
    default:
        epd_fill_rect_i((int16_t)(x - 5), (int16_t)(y - 5), 3u, 3u);
        epd_fill_rect_i((int16_t)(x - 8), (int16_t)(y - 8), 3u, 3u);
        epd_fill_rect_i((int16_t)(x - 11), (int16_t)(y - 11), 3u, 3u);
        break;
    }

    epd_fill_rect_i((int16_t)(x + tx[frame] - 2), (int16_t)(y + ty[frame] - 2), 5u, 5u);
}

static void epd_draw_char(uint16_t x, uint16_t y, char c, uint8_t scale)
{
    uint8_t col;
    uint8_t row;
    uint8_t sx;
    uint8_t sy;
    uint8_t bits;
    uint8_t glyph_index;

    if ((uint8_t)c < 32u || (uint8_t)c > 127u) {
        c = '?';
    }
    glyph_index = (uint8_t)c - 32u;

    for (col = 0; col < 5u; col++) {
        bits = font5x7[glyph_index][col];
        for (row = 0; row < 7u; row++) {
            if (bits & (1u << row)) {
                for (sx = 0; sx < scale; sx++) {
                    for (sy = 0; sy < scale; sy++) {
                        epd_pixel((uint16_t)(x + col * scale + sx),
                                  (uint16_t)(y + row * scale + sy), 1);
                    }
                }
            }
        }
    }
}

static void epd_draw_string(uint16_t x, uint16_t y, const char *s, uint8_t scale)
{
    uint16_t cursor;
    uint16_t step;

    cursor = x;
    step = (uint16_t)(6u * scale);
    while (*s && cursor < EPD_SCREEN_W) {
        epd_draw_char(cursor, y, *s++, scale);
        cursor = (uint16_t)(cursor + step);
    }
}

static void epd_flush_partial(void)
{
    (void)epd_write_buffer_to_screen_partial(epd_buf);
}

static void epd_alt_clear_buffer(void)
{
    uint16_t i;

    for (i = 0; i < EPD_ALT_BUF_SIZE; i++) {
        epd_buf[i] = 0xFF;
    }
}

void epd_full_refresh_once(void)
{
    if (g_epd_driver == EPD_DRIVER_SPD2701) {
        epd_alt_init();
        epd_alt_clear_buffer();
        (void)epd_alt_write_buffer_to_screen(epd_buf);
    } else {
        epd_ssd_init_controller_and_clear();
    }

    g_epd_render_dirty = 1;
}

static void epd_alt_pixel(uint16_t x, uint16_t y, uint8_t black)
{
    uint16_t index;
    uint8_t mask;

    if (x >= EPD_ALT_SCREEN_W || y >= EPD_ALT_SCREEN_H) {
        return;
    }

    index = (uint16_t)(x * EPD_ALT_RAM_W_BYTES + (y >> 3));
    mask = (uint8_t)(0x80u >> (y & 7u));

    if (black) {
        epd_buf[index] &= (uint8_t)(~mask);
    } else {
        epd_buf[index] |= mask;
    }
}

static void epd_alt_fill_rect_i(int16_t x, int16_t y, uint8_t w, uint8_t h)
{
    uint8_t dx;
    uint8_t dy;
    int16_t px;
    int16_t py;

    for (dy = 0; dy < h; dy++) {
        py = (int16_t)(y + dy);
        if (py < 0) {
            continue;
        }
        for (dx = 0; dx < w; dx++) {
            px = (int16_t)(x + dx);
            if (px >= 0) {
                epd_alt_pixel((uint16_t)px, (uint16_t)py, 1);
            }
        }
    }
}

static void epd_alt_draw_status_anim(uint16_t cx, uint16_t cy, uint8_t frame)
{
    static const int8_t tx[8] = {0, 9, 12, 9, 0, -9, -12, -9};
    static const int8_t ty[8] = {-12, -9, 0, 9, 12, 9, 0, -9};
    uint8_t prev;
    int16_t x;
    int16_t y;

    frame &= 0x07u;
    prev = (uint8_t)((frame + 7u) & 0x07u);
    x = (int16_t)cx;
    y = (int16_t)cy;

    epd_alt_fill_rect_i((int16_t)(x - 15), (int16_t)(y - 15), 8u, 2u);
    epd_alt_fill_rect_i((int16_t)(x - 15), (int16_t)(y - 15), 2u, 8u);
    epd_alt_fill_rect_i((int16_t)(x + 8), (int16_t)(y - 15), 8u, 2u);
    epd_alt_fill_rect_i((int16_t)(x + 14), (int16_t)(y - 15), 2u, 8u);
    epd_alt_fill_rect_i((int16_t)(x - 15), (int16_t)(y + 14), 8u, 2u);
    epd_alt_fill_rect_i((int16_t)(x - 15), (int16_t)(y + 8), 2u, 8u);
    epd_alt_fill_rect_i((int16_t)(x + 8), (int16_t)(y + 14), 8u, 2u);
    epd_alt_fill_rect_i((int16_t)(x + 14), (int16_t)(y + 8), 2u, 8u);

    epd_alt_fill_rect_i((int16_t)(x - 2), (int16_t)(y - 2), 5u, 5u);
    epd_alt_fill_rect_i((int16_t)(x - 7), y, 3u, 1u);
    epd_alt_fill_rect_i((int16_t)(x + 5), y, 3u, 1u);
    epd_alt_fill_rect_i(x, (int16_t)(y - 7), 1u, 3u);
    epd_alt_fill_rect_i(x, (int16_t)(y + 5), 1u, 3u);

    epd_alt_fill_rect_i((int16_t)(x + tx[prev] - 1), (int16_t)(y + ty[prev] - 1), 3u, 3u);

    switch (frame) {
    case 0:
        epd_alt_fill_rect_i((int16_t)(x - 1), (int16_t)(y - 13), 3u, 11u);
        break;
    case 1:
        epd_alt_fill_rect_i((int16_t)(x + 3), (int16_t)(y - 5), 3u, 3u);
        epd_alt_fill_rect_i((int16_t)(x + 6), (int16_t)(y - 8), 3u, 3u);
        epd_alt_fill_rect_i((int16_t)(x + 9), (int16_t)(y - 11), 3u, 3u);
        break;
    case 2:
        epd_alt_fill_rect_i((int16_t)(x + 2), (int16_t)(y - 1), 11u, 3u);
        break;
    case 3:
        epd_alt_fill_rect_i((int16_t)(x + 3), (int16_t)(y + 3), 3u, 3u);
        epd_alt_fill_rect_i((int16_t)(x + 6), (int16_t)(y + 6), 3u, 3u);
        epd_alt_fill_rect_i((int16_t)(x + 9), (int16_t)(y + 9), 3u, 3u);
        break;
    case 4:
        epd_alt_fill_rect_i((int16_t)(x - 1), (int16_t)(y + 2), 3u, 11u);
        break;
    case 5:
        epd_alt_fill_rect_i((int16_t)(x - 5), (int16_t)(y + 3), 3u, 3u);
        epd_alt_fill_rect_i((int16_t)(x - 8), (int16_t)(y + 6), 3u, 3u);
        epd_alt_fill_rect_i((int16_t)(x - 11), (int16_t)(y + 9), 3u, 3u);
        break;
    case 6:
        epd_alt_fill_rect_i((int16_t)(x - 13), (int16_t)(y - 1), 11u, 3u);
        break;
    default:
        epd_alt_fill_rect_i((int16_t)(x - 5), (int16_t)(y - 5), 3u, 3u);
        epd_alt_fill_rect_i((int16_t)(x - 8), (int16_t)(y - 8), 3u, 3u);
        epd_alt_fill_rect_i((int16_t)(x - 11), (int16_t)(y - 11), 3u, 3u);
        break;
    }

    epd_alt_fill_rect_i((int16_t)(x + tx[frame] - 2), (int16_t)(y + ty[frame] - 2), 5u, 5u);
}

static void epd_alt_draw_char(uint16_t x, uint16_t y, char c, uint8_t scale)
{
    uint8_t col;
    uint8_t row;
    uint8_t sx;
    uint8_t sy;
    uint8_t bits;
    uint8_t glyph_index;

    if ((uint8_t)c < 32u || (uint8_t)c > 127u) {
        c = '?';
    }
    glyph_index = (uint8_t)c - 32u;

    for (col = 0; col < 5u; col++) {
        bits = font5x7[glyph_index][col];
        for (row = 0; row < 7u; row++) {
            if (bits & (1u << row)) {
                for (sx = 0; sx < scale; sx++) {
                    for (sy = 0; sy < scale; sy++) {
                        epd_alt_pixel((uint16_t)(x + col * scale + sx),
                                      (uint16_t)(y + row * scale + sy), 1);
                    }
                }
            }
        }
    }
}

static void epd_alt_draw_string(uint16_t x, uint16_t y, const char *s, uint8_t scale)
{
    uint16_t cursor;
    uint16_t step;

    cursor = x;
    step = (uint16_t)(6u * scale);
    while (*s && cursor < EPD_ALT_SCREEN_W) {
        epd_alt_draw_char(cursor, y, *s++, scale);
        cursor = (uint16_t)(cursor + step);
    }
}

static void sample_to_lines(const TempSample *s, char line[4][24])
{
    char *p;

    p = line[0];
    p = append_str(p, "DIE ");
    p = append_t10(p, s->die_t10);
    (void)append_str(p, " C");

    p = line[1];
    p = append_str(p, "NTC ");
    p = append_t10(p, s->ntc_t10);
    (void)append_str(p, " C");

    p = line[2];
    p = append_str(p, "TMP ");
    p = append_t10(p, s->tmp_local_t10);
    (void)append_str(p, " C");

    p = line[3];
    p = append_str(p, "ALM ");
    p = append_t10(p, app_threshold_t10());
    (void)append_str(p, " C");
}

static void setting_to_line(uint8_t item, uint8_t selected, uint8_t editing, char *line)
{
    char *p;

    p = line;
    if (selected) {
        *p++ = editing ? '*' : '>';
    } else {
        *p++ = ' ';
    }
    *p++ = ' ';

    switch (item) {
    case 0:
        p = append_str(p, "SAMPLE ");
        p = append_u16(p, app_sample_interval());
        (void)append_str(p, "s");
        break;
    case 1:
        p = append_str(p, "ALM TEMP ");
        p = append_t10(p, app_threshold_t10());
        (void)append_str(p, "C");
        break;
    case 2:
        p = append_str(p, "STORE ");
        (void)append_u16(p, app_storage_limit());
        break;
    default:
        p = append_str(p, "ALM TIME ");
        p = append_u16(p, app_alarm_duration_seconds());
        (void)append_str(p, "s");
        break;
    }
}

static void epd_alt_show_settings(void)
{
    uint8_t i;
    char line[28];

    epd_alt_clear_buffer();
    epd_alt_draw_string(0, 0, "SETTINGS", 2);
    for (i = 0; i < 4u; i++) {
        setting_to_line(i, (uint8_t)(i == g_epd_settings_selected), g_epd_settings_editing, line);
        epd_alt_draw_string(0, (uint16_t)(28u + i * 20u), line, 1);
    }
    (void)epd_alt_write_buffer_to_screen(epd_buf);
}

static void epd_show_settings(void)
{
    uint8_t i;
    char line[28];

    epd_clear_buffer();
    epd_draw_string(0, 0, "SETTINGS", 2);
    for (i = 0; i < 4u; i++) {
        setting_to_line(i, (uint8_t)(i == g_epd_settings_selected), g_epd_settings_editing, line);
        epd_draw_string(0, (uint16_t)(28u + i * 20u), line, 2);
    }
    epd_flush_partial();
}

static void epd_alt_show_current(const TempSample *s)
{
    char lines[4][24];
    uint8_t i;
    uint8_t frame;

    frame = epd_next_anim_frame();
    sample_to_lines(s, lines);
    epd_alt_clear_buffer();
    epd_alt_draw_string(0, 0, "TEMP LOGGER", 2);
    for (i = 0; i < 4u; i++) {
        epd_alt_draw_string(0, (uint16_t)(22u + i * 18u), lines[i], 2);
    }
    epd_alt_draw_status_anim(148u, 54u, frame);
    if (sample_over_threshold(s)) {
        epd_alt_draw_string(92, 124, "ALM 1", 2);
    } else {
        epd_alt_draw_string(92, 124, "ALM 0", 2);
    }
    (void)epd_alt_write_buffer_to_screen(epd_buf);
}

static void epd_show_current(const TempSample *s)
{
    char lines[4][24];
    uint8_t i;
    uint8_t frame;

    frame = epd_next_anim_frame();
    sample_to_lines(s, lines);
    epd_clear_buffer();
    epd_draw_string(0, 0, "TEMP LOGGER", 2);
    for (i = 0; i < 4u; i++) {
        epd_draw_string(0, (uint16_t)(22u + i * 18u), lines[i], 2);
    }
    epd_draw_status_anim(222u, 50u, frame);
    if (sample_over_threshold(s)) {
        epd_draw_string(154, 104, "ALM 1", 2);
    } else {
        epd_draw_string(154, 104, "ALM 0", 2);
    }
    epd_flush_partial();
}

void epd_force_next_current_refresh(void)
{
    if (g_epd_has_target_sample) {
        g_epd_render_view = EPD_VIEW_CURRENT;
    }
    g_epd_render_dirty = 1;
}

void epd_show_current_auto(const TempSample *s)
{
    if (!g_epd_auto_update) {
        return;
    }

    g_epd_target_sample = *s;
    g_epd_has_target_sample = 1;
    g_epd_render_view = EPD_VIEW_CURRENT;
    g_epd_render_dirty = 1;
}

static void record_to_line(const TempRecord *r, char *line)
{
    char *p;

    p = line;
    *p++ = '#';
    p = append_u16(p, r->seq);
    p = append_str(p, " I");
    p = append_t10(p, r->die_t10);
    p = append_str(p, " N");
    p = append_t10(p, r->ntc_t10);
    p = append_str(p, " T");
    (void)append_t10(p, r->tmp_local_t10);
}

static void epd_alt_show_history(uint16_t start)
{
    uint16_t count;
    uint8_t row;
    TempRecord r;
    char line[40];
    char *p;

    count = history_count();
    if (start >= count && count > 0) {
        start = 0;
    }

    epd_alt_clear_buffer();
    p = line;
    p = append_str(p, "HISTORY ");
    p = append_u16(p, count);
    (void)append_str(p, " REC");
    epd_alt_draw_string(0, 0, line, 2);

    if (count == 0) {
        epd_alt_draw_string(0, 44, "NO RECORD", 2);
        (void)epd_alt_write_buffer_to_screen(epd_buf);
        return;
    }

    for (row = 0; row < HISTORY_ROWS_ON_EPD; row++) {
        if (history_get((uint16_t)(start + row), &r)) {
            record_to_line(&r, line);
            epd_alt_draw_string(0, (uint16_t)(24u + row * 18u), line, 1);
        }
    }

    (void)epd_alt_write_buffer_to_screen(epd_buf);
}

static void epd_show_history(uint16_t start)
{
    uint16_t count;
    uint8_t row;
    TempRecord r;
    char line[40];
    char *p;

    count = history_count();
    if (start >= count && count > 0) {
        start = 0;
    }

    epd_clear_buffer();
    p = line;
    p = append_str(p, "HISTORY ");
    p = append_u16(p, count);
    (void)append_str(p, " REC");
    epd_draw_string(0, 0, line, 2);

    if (count == 0) {
        epd_draw_string(0, 44, "NO RECORD", 2);
        epd_flush_partial();
        return;
    }

    for (row = 0; row < HISTORY_ROWS_ON_EPD; row++) {
        if (history_get((uint16_t)(start + row), &r)) {
            record_to_line(&r, line);
            epd_draw_string(0, (uint16_t)(24u + row * 18u), line, 1);
        }
    }

    epd_flush_partial();
}

void epd_show_history_page(uint16_t start)
{
    g_epd_target_history_start = start;
    g_epd_auto_update = 0;
    g_epd_render_view = EPD_VIEW_HISTORY;
    g_epd_render_dirty = 1;
}

void epd_show_settings_page(uint8_t selected, uint8_t editing)
{
    if (selected > 3u) {
        selected = 3u;
    }
    g_epd_settings_selected = selected;
    g_epd_settings_editing = editing ? 1u : 0u;
    g_epd_auto_update = 0;
    g_epd_render_view = EPD_VIEW_SETTINGS;
    g_epd_render_dirty = 1;
}

void epd_render_task(void)
{
    if (!g_epd_render_dirty) {
        return;
    }

    g_epd_render_dirty = 0;

    if (g_epd_render_view == EPD_VIEW_HISTORY) {
        if (g_epd_driver == EPD_DRIVER_SPD2701) {
            epd_alt_show_history(g_epd_target_history_start);
        } else {
            epd_show_history(g_epd_target_history_start);
        }
    } else if (g_epd_render_view == EPD_VIEW_SETTINGS) {
        if (g_epd_driver == EPD_DRIVER_SPD2701) {
            epd_alt_show_settings();
        } else {
            epd_show_settings();
        }
    } else if (g_epd_has_target_sample) {
        if (g_epd_driver == EPD_DRIVER_SPD2701) {
            epd_alt_show_current(&g_epd_target_sample);
        } else {
            epd_show_current(&g_epd_target_sample);
        }
        if (g_epd_auto_update) {
            g_epd_render_dirty = 1;
        }
    }
}

uint8_t epd_render_pending(void)
{
    return g_epd_render_dirty;
}

uint8_t epd_auto_enabled(void)
{
    return g_epd_auto_update;
}

void epd_resume_auto(void)
{
    g_epd_auto_update = 1;
    epd_force_next_current_refresh();
}

void epd_use_ssd(void)
{
    g_epd_driver = EPD_DRIVER_SSD1673;
    epd_force_next_current_refresh();
}

void epd_use_alt_auto(void)
{
    g_epd_driver = EPD_DRIVER_SPD2701;
    g_epd_auto_update = 1;
    epd_alt_init();
    epd_force_next_current_refresh();
}
