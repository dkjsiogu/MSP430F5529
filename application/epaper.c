#include "epaper.h"
#include "app_state.h"
#include "board.h"
#include "flash_log.h"
#include "format.h"
#include "app_resources.h"
#include "text_reader.h"
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

#define EPD_VIEW_CURRENT            0u /* 当前温度主界面渲染目标。 */
#define EPD_VIEW_HISTORY            1u /* Flash 历史记录界面渲染目标。 */
#define EPD_VIEW_SETTINGS           2u /* 设置界面渲染目标。 */
#define EPD_VIEW_GIF                3u /* 全屏 GIF 动画播放界面渲染目标。 */
#define EPD_VIEW_TEXT               4u /* 文本阅读界面渲染目标。 */
#define EPD_SETTINGS_ROWS           5u /* 设置页面显示的配置项数量。 */
#define EPD_HOURGLASS_X             198 /* SSD1673 主界面沙漏默认 X 坐标，资源索引缺失时使用。 */
#define EPD_HOURGLASS_Y             12  /* SSD1673 主界面沙漏默认 Y 坐标，资源索引缺失时使用。 */
#define EPD_ALT_HOURGLASS_X         120 /* 备用驱动主界面沙漏 X 坐标。 */
#define EPD_ALT_HOURGLASS_Y         4   /* 备用驱动主界面沙漏 Y 坐标。 */
#define EPD_ZHENG_X                 170 /* SSD1673 主界面“郑”字绘制 X 坐标，位于沙漏左侧。 */
#define EPD_ZHENG_Y                 38  /* SSD1673 主界面“郑”字绘制 Y 坐标。 */
#define EPD_ALT_ZHENG_X             94  /* 备用驱动主界面“郑”字绘制 X 坐标。 */
#define EPD_ALT_ZHENG_Y             96  /* 备用驱动主界面“郑”字绘制 Y 坐标。 */
#define EPD_TEXT_TOP_Y              14u /* SSD1673 阅读页正文起始 Y 坐标。 */
#define EPD_TEXT_ALT_TOP_Y          12u /* 备用驱动阅读页正文起始 Y 坐标。 */
#define EPD_TEXT_LINES              6u  /* SSD1673 阅读页正文行数。 */
#define EPD_TEXT_ALT_LINES          7u  /* 备用驱动阅读页正文行数。 */

static uint8_t g_epd_auto_update = 1;
static uint8_t g_epd_driver = EPD_DRIVER_SSD1673;
static uint8_t g_epd_render_view = EPD_VIEW_CURRENT;
static uint8_t g_epd_render_dirty = 1;
static uint8_t g_epd_has_target_sample = 0;
static uint8_t g_epd_settings_selected = 0;
static uint8_t g_epd_settings_editing = 0;
static uint8_t g_epd_history_playback = 0;
static uint8_t g_epd_gif_playback = 0;
static uint16_t g_epd_target_history_start = 0;
static uint16_t g_epd_last_auto_frame_tick = 0;
static uint16_t g_epd_last_history_scroll_tick = 0;
static uint16_t g_epd_last_gif_frame_tick = 0;
static uint16_t g_epd_last_render_tick = 0;
static uint8_t g_epd_has_render_tick = 0;
static TempSample g_epd_target_sample;
static uint8_t epd_buf[EPD_BUF_SIZE];
static uint8_t g_resource_row_buf[APP_RESOURCE_ROW_MAX_BYTES];
static uint8_t g_mascot_frame_index = 0;
static uint8_t g_gif_image_id = APP_RESOURCE_IMAGE_MASCOT;
static uint16_t g_hourglass_cycle_start_tick = 0;
static int32_t g_tmp_avg_sum_t10 = 0;
static uint8_t g_tmp_avg_count = 0;
static int16_t g_tmp_avg_display_t10 = INVALID_T10;

/* 前置声明 SSD1673 字符串绘制函数，供沙漏字幕提前调用。 */
static void epd_draw_string(uint16_t x, uint16_t y, const char *s, uint8_t scale);

/* 前置声明备用驱动字符串绘制函数，供沙漏字幕提前调用。 */
static void epd_alt_draw_string(uint16_t x, uint16_t y, const char *s, uint8_t scale);

/* 计算当前沙漏周期对应的 10ms 节拍数。 */
static uint16_t epd_hourglass_period_ticks(void)
{
    return (uint16_t)((uint16_t)app_hourglass_seconds() * BOARD_TICKS_PER_SECOND);
}

/* 根据当前累计的 TMP 样本计算平均温度。 */
static int16_t epd_hourglass_current_avg_t10(void)
{
    int32_t rounded;

    if (g_tmp_avg_count == 0) {
        return INVALID_T10;
    }

    rounded = g_tmp_avg_sum_t10;
    if (rounded >= 0) {
        rounded += (int32_t)(g_tmp_avg_count / 2u);
    } else {
        rounded -= (int32_t)(g_tmp_avg_count / 2u);
    }
    return (int16_t)(rounded / (int32_t)g_tmp_avg_count);
}

/* 结束当前统计窗口，并把最后一次有效平均值保留给界面显示。 */
static void epd_hourglass_reset_avg_window(void)
{
    if (g_tmp_avg_count > 0) {
        g_tmp_avg_display_t10 = epd_hourglass_current_avg_t10();
    }
    g_tmp_avg_sum_t10 = 0;
    g_tmp_avg_count = 0;
}

/* 根据系统时间推进沙漏周期，周期结束时同步重置 TMP 平均窗口。 */
static uint16_t epd_hourglass_elapsed_ticks(void)
{
    uint16_t elapsed;
    uint16_t period_ticks;

    period_ticks = epd_hourglass_period_ticks();
    elapsed = (uint16_t)(board_tick10() - g_hourglass_cycle_start_tick);
    if (elapsed >= period_ticks) {
        g_hourglass_cycle_start_tick = board_tick10();
        epd_hourglass_reset_avg_window();
        return 0;
    }
    return elapsed;
}

/* 把新的 TMP 样本加入当前沙漏周期的平均温度统计。 */
static void epd_hourglass_add_sample(const TempSample *s)
{
    (void)epd_hourglass_elapsed_ticks();
    if (!temp_is_valid(s->tmp_local_t10)) {
        return;
    }

    if (g_tmp_avg_count < 250u) {
        g_tmp_avg_sum_t10 += s->tmp_local_t10;
        g_tmp_avg_count++;
    }
    g_tmp_avg_display_t10 = epd_hourglass_current_avg_t10();
}

/* 判断当前沙漏是否处于周期末尾的翻转阶段，并返回翻转帧号。 */
static uint8_t epd_hourglass_flip_phase(uint16_t elapsed_ticks, uint16_t period_ticks)
{
    uint16_t flip_start;
    uint16_t flip_elapsed;
    uint16_t flip_ticks;

    flip_ticks = HOURGLASS_FLIP_TICKS;
    if (flip_ticks >= period_ticks) {
        flip_ticks = (uint16_t)(period_ticks / 3u);
    }
    if (flip_ticks == 0) {
        return 0;
    }

    flip_start = (uint16_t)(period_ticks - flip_ticks);
    if (elapsed_ticks < flip_start) {
        return 0;
    }

    flip_elapsed = (uint16_t)(elapsed_ticks - flip_start);
    return (uint8_t)(1u + (uint8_t)(((uint32_t)flip_elapsed * 3u) / flip_ticks));
}

/* 标记墨水屏有新的目标画面需要渲染。 */
static void epd_request_render(uint8_t urgent)
{
    (void)urgent;
    g_epd_render_dirty = 1;
}

/* 判断当前是否允许向墨水屏提交下一帧，按配置统一限制屏幕帧率。 */
static uint8_t epd_render_frame_ready(void)
{
    if (!g_epd_has_render_tick) {
        return 1;
    }
    return board_tick10_elapsed(g_epd_last_render_tick, EPD_RENDER_MIN_TICKS);
}

/* 记录最近一次实际刷屏时间，让按键响应和墨水屏帧率解耦。 */
static void epd_note_render_frame(void)
{
    g_epd_has_render_tick = 1;
    g_epd_last_render_tick = board_tick10();
}

/* 判断主界面自动动画帧是否到达刷新间隔。 */
static uint8_t epd_auto_frame_due(void)
{
    if (!g_epd_auto_update || !g_epd_has_target_sample) {
        return 0;
    }
    return board_tick10_elapsed(g_epd_last_auto_frame_tick, EPD_AUTO_FRAME_TICKS);
}

/* 判断全屏 GIF 播放界面是否需要推进到下一帧。 */
static uint8_t epd_gif_frame_due(void)
{
    if (!g_epd_gif_playback || g_epd_render_view != EPD_VIEW_GIF) {
        return 0;
    }
    return board_tick10_elapsed(g_epd_last_gif_frame_tick, EPD_AUTO_FRAME_TICKS);
}

/* 根据历史记录数量计算自动播放时优先显示最近记录的起始下标。 */
static uint16_t epd_history_latest_start(uint16_t count)
{
    if (count <= HISTORY_ROWS_ON_EPD) {
        return 0;
    }
    return (uint16_t)(count - HISTORY_ROWS_ON_EPD);
}

/* 判断历史记录自动播放是否需要滚动到下一页。 */
static uint8_t epd_history_frame_due(void)
{
    uint16_t count;

    if (!g_epd_history_playback || g_epd_render_view != EPD_VIEW_HISTORY) {
        return 0;
    }
    if (!board_tick10_elapsed(g_epd_last_history_scroll_tick, EPD_HISTORY_SCROLL_TICKS)) {
        return 0;
    }

    count = history_count();
    if (count == 0 || count <= HISTORY_ROWS_ON_EPD) {
        g_epd_target_history_start = 0;
    } else {
        g_epd_target_history_start++;
        if (g_epd_target_history_start >= count) {
            g_epd_target_history_start = 0;
        }
    }
    g_epd_last_history_scroll_tick = board_tick10();
    return 1;
}

/* 将墨水屏 SPI 控制线恢复到空闲电平。 */
static void epd_bus_idle(void)
{
    EPD_CS_OUT |= EPD_CS_BIT;
    EPD_CLK_OUT &= ~EPD_CLK_BIT;
    EPD_SDI_OUT &= ~EPD_SDI_BIT;
    EPD_DC_OUT |= EPD_DC_BIT;
}

/* 初始化墨水屏使用的 GPIO 和软件 SPI 引脚方向。 */
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

/* 通过软件 SPI 向墨水屏发送一个字节。 */
static void spi0_write(uint8_t value)
{
    uint8_t bit;

    for (bit = 0; bit < 8u; bit++) {
        if (value & 0x80u) {
            EPD_SDI_OUT |= EPD_SDI_BIT;
        } else {
            EPD_SDI_OUT &= ~EPD_SDI_BIT;
        }
        EPD_CLK_OUT |= EPD_CLK_BIT;
        EPD_CLK_OUT &= ~EPD_CLK_BIT;
        value = (uint8_t)(value << 1);
    }
}

/* 控制墨水屏片选信号，selected 为 1 时选中屏幕。 */
static void epd_select(uint8_t selected)
{
    if (selected) {
        EPD_CS_OUT &= ~EPD_CS_BIT;
    } else {
        EPD_CS_OUT |= EPD_CS_BIT;
    }
}

/* 向墨水屏控制器发送一个命令字节。 */
static void epd_cmd(uint8_t cmd)
{
    epd_select(0);
    epd_select(1);
    EPD_CLK_OUT &= ~EPD_CLK_BIT;
    EPD_DC_OUT &= ~EPD_DC_BIT;
    spi0_write(cmd);
    epd_select(0);
}

/* 向墨水屏控制器发送一个数据字节。 */
static void epd_data(uint8_t data)
{
    epd_select(0);
    epd_select(1);
    EPD_CLK_OUT &= ~EPD_CLK_BIT;
    EPD_DC_OUT |= EPD_DC_BIT;
    spi0_write(data);
    epd_select(0);
}

/* 进入连续数据写入状态，减少每字节切片选的开销。 */
static void epd_data_stream_start(void)
{
    epd_select(0);
    epd_select(1);
    EPD_CLK_OUT &= ~EPD_CLK_BIT;
    EPD_DC_OUT |= EPD_DC_BIT;
}

/* 在连续数据写入状态下发送一个数据字节。 */
static void epd_data_stream_write(uint8_t data)
{
    spi0_write(data);
}

/* 结束连续数据写入并释放片选。 */
static void epd_data_stream_end(void)
{
    epd_select(0);
}

/* 等待 BUSY 引脚释放，超时返回 0。 */
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

/* 触发刷新后等待 BUSY 完成，并补充控制器稳定等待时间。 */
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

/* 对墨水屏执行硬件复位时序。 */
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

/* 发送控制器软复位命令并等待其恢复可用。 */
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

/* 在无法确定上电状态时重新初始化总线并复位控制器。 */
static void epd_recover_from_unknown_state(void)
{
    spi0_init();
    delay_ms(EPD_STARTUP_SETTLE_MS);
    epd_reset();
    (void)epd_wait_busy(EPD_BUSY_TIMEOUT_MS);
    (void)epd_soft_reset();
    (void)epd_wait_busy(EPD_BUSY_TIMEOUT_MS);
}

/* 写入 SSD1673 全屏刷新波形表。 */
static void epd_write_lut(void)
{
    uint8_t i;

    epd_cmd(0x32);
    epd_data_stream_start();
    for (i = 0; i < sizeof(epd_lut); i++) {
        epd_data_stream_write(epd_lut[i]);
    }
    epd_data_stream_end();
}

/* 写入 SSD1673 局部刷新波形表。 */
static void epd_write_partial_lut(void)
{
    uint8_t i;

    epd_cmd(0x32);
    epd_data_stream_start();
    for (i = 0; i < sizeof(epd_partial_lut); i++) {
        epd_data_stream_write(epd_partial_lut[i]);
    }
    epd_data_stream_end();
}

/* 写入备用墨水屏驱动使用的波形表。 */
static void epd_alt_write_lut(void)
{
    uint8_t i;

    epd_cmd(0x32);
    epd_data_stream_start();
    for (i = 0; i < sizeof(epd_alt_lut); i++) {
        epd_data_stream_write(epd_alt_lut[i]);
    }
    epd_data_stream_end();
}

/* 设置 SSD1673 的原生 RAM 写入窗口。 */
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

/* 将 SSD1673 RAM 写入游标移动到窗口起点。 */
static void epd_set_ram_cursor(void)
{
    epd_cmd(0x4E);
    epd_data(0x00);
    epd_cmd(0x4F);
    epd_data(0xF9);
}

/* 设置备用驱动的 RAM 写入窗口和游标。 */
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

/* 使用指定刷新控制字触发一次墨水屏刷新。 */
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

/* 对 SSD1673 执行一次全屏刷新。 */
static uint8_t epd_update(void)
{
    return epd_update_with_ctrl(EPD_UPDATE_CTRL_FULL, EPD_FULL_POST_UPDATE_MS, EPD_FULL_POST_UPDATE_MS);
}

/* 对备用驱动执行一次局部刷新，并让控制器回到待机刷新状态。 */
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

/* 准备向 SSD1673 新图像 RAM 写入数据。 */
static void epd_write_new_ram_start(void)
{
    epd_set_ram_area();
    epd_set_ram_cursor();
    delay_ms(1);
    epd_cmd(0x24);
    delay_ms(1);
}

/* 用同一个字节填充 SSD1673 新图像 RAM。 */
static void epd_write_new_ram_fill_only(uint8_t value)
{
    uint16_t col;
    uint16_t row;

    epd_write_new_ram_start();
    epd_data_stream_start();
    for (col = 0; col < EPD_RAM_H; col++) {
        for (row = 0; row < EPD_RAM_W_BYTES; row++) {
            epd_data_stream_write(value);
        }
    }
    epd_data_stream_end();
}

/* 将帧缓冲内容写入 SSD1673 新图像 RAM，但暂不触发刷新。 */
static void epd_write_new_ram_buffer_only(const uint8_t *buf)
{
    uint16_t i;

    epd_write_new_ram_start();
    epd_data_stream_start();
    for (i = 0; i < EPD_BUF_SIZE; i++) {
        epd_data_stream_write(buf[i]);
    }
    epd_data_stream_end();
}

/* 将帧缓冲内容写入备用驱动的图像 RAM，但暂不触发刷新。 */
static void epd_alt_write_ram_buffer_only(const uint8_t *buf)
{
    uint16_t i;

    epd_alt_set_ram_area();
    epd_cmd(0x24);
    epd_data_stream_start();
    for (i = 0; i < EPD_ALT_BUF_SIZE; i++) {
        epd_data_stream_write(buf[i]);
    }
    epd_data_stream_end();
}

/* 将当前帧缓冲通过 SSD1673 局部刷新送到屏幕。 */
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

/* 将当前帧缓冲送到备用驱动并触发刷新。 */
static uint8_t epd_alt_write_buffer_to_screen(const uint8_t *buf)
{
    epd_alt_write_ram_buffer_only(buf);
    return epd_alt_update();
}

/* 初始化 SSD1673 控制器并清成白底，为后续局部刷新建立基准。 */
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
    g_epd_history_playback = 0;
    g_epd_gif_playback = 0;
    g_epd_last_auto_frame_tick = board_tick10();
    g_epd_last_history_scroll_tick = board_tick10();
    g_epd_last_gif_frame_tick = board_tick10();
    g_epd_last_render_tick = board_tick10();
    g_epd_has_render_tick = 0;
    g_hourglass_cycle_start_tick = board_tick10();
    g_mascot_frame_index = 0;
    g_gif_image_id = APP_RESOURCE_IMAGE_MASCOT;
    g_tmp_avg_sum_t10 = 0;
    g_tmp_avg_count = 0;
    g_tmp_avg_display_t10 = INVALID_T10;
}

/* 初始化备用墨水屏控制器。 */
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

/* 清空 SSD1673 渲染帧缓冲，0xFF 表示白色像素。 */
static void epd_clear_buffer(void)
{
    uint16_t i;

    for (i = 0; i < EPD_BUF_SIZE; i++) {
        epd_buf[i] = 0xFF;
    }
}

/* 在 SSD1673 逻辑坐标中写入一个像素。 */
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

/* 在 SSD1673 帧缓冲中填充一个小矩形，用于局部清出调试标识背景。 */
static void epd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t black)
{
    uint16_t px;
    uint16_t py;

    for (py = y; py < (uint16_t)(y + h) && py < EPD_SCREEN_H; py++) {
        for (px = x; px < (uint16_t)(x + w) && px < EPD_SCREEN_W; px++) {
            epd_pixel(px, py, black);
        }
    }
}

/* 根据沙漏周期时间选择图片关键帧，末尾按翻转时长映射到最后几帧。 */
static uint8_t epd_hourglass_asset_frame_index(uint8_t frame_count)
{
    uint16_t elapsed;
    uint16_t period_ticks;
    uint16_t normal_count;
    uint8_t phase;
    uint8_t index;

    if (frame_count == 0) {
        return 0;
    }

    elapsed = epd_hourglass_elapsed_ticks();
    period_ticks = epd_hourglass_period_ticks();
    if (period_ticks == 0) {
        return 0;
    }
    normal_count = frame_count;
    if (normal_count > 3u) {
        normal_count = (uint16_t)(normal_count - 3u);
    }
    if (normal_count == 0) {
        return 0;
    }

    phase = epd_hourglass_flip_phase(elapsed, period_ticks);
    if (phase > 0 && frame_count > normal_count) {
        index = (uint8_t)(normal_count + phase - 1u);
        if (index >= frame_count) {
            index = (uint8_t)(frame_count - 1u);
        }
        return index;
    }

    index = (uint8_t)(((uint32_t)elapsed * normal_count) / period_ticks);
    if (index >= normal_count) {
        index = (uint8_t)(normal_count - 1u);
    }
    return index;
}

/* 把一行 1bpp 数据贴到 SSD1673 帧缓冲。 */
static void epd_draw_1bpp_row(uint16_t width, const uint8_t *row, uint16_t py,
                              int16_t x, int16_t y, uint8_t scale)
{
    uint16_t px;
    uint8_t bit;
    uint8_t sx;
    uint8_t sy;

    if (scale == 0) {
        scale = 1;
    }

    for (px = 0; px < width; px++) {
        bit = (uint8_t)(0x80u >> (px & 7u));
        if ((row[px >> 3] & bit) == 0) {
            continue;
        }
        for (sy = 0; sy < scale; sy++) {
            for (sx = 0; sx < scale; sx++) {
                epd_pixel((uint16_t)(x + (int16_t)(px * scale + sx)),
                          (uint16_t)(y + (int16_t)(py * scale + sy)), 1);
            }
        }
    }
}

/* 从当前图片资源按行读取并贴到 SSD1673 帧缓冲。 */
static uint8_t epd_draw_image_rows(const AppImageInfo *info, int16_t x, int16_t y)
{
    uint16_t py;
    uint8_t scale;

    scale = info->default_scale;
    if (scale == 0) {
        scale = 1;
    }

    for (py = 0; py < info->height; py++) {
        if (!app_resources_read_image_row(g_resource_row_buf, info->stride)) {
            return 0;
        }
        epd_draw_1bpp_row(info->width, g_resource_row_buf, py, x, y, scale);
    }
    return 1;
}

/* 按帧数循环推进图片帧游标。 */
static uint8_t epd_next_frame_index(uint8_t frame_count, uint8_t *cursor)
{
    uint8_t index;

    if (frame_count == 0) {
        return 0;
    }
    if (*cursor >= frame_count) {
        *cursor = 0;
    }

    index = *cursor;
    *cursor = (uint8_t)(*cursor + 1u);
    if (*cursor >= frame_count) {
        *cursor = 0;
    }
    return index;
}

/* 计算指定尺寸图片在屏幕单方向上的居中坐标。 */
static int16_t epd_center_coord(uint16_t image_size, uint16_t screen_size, uint8_t scale)
{
    if (scale == 0) {
        scale = 1;
    }
    image_size = (uint16_t)(image_size * scale);
    if (image_size >= screen_size) {
        return 0;
    }
    return (int16_t)((screen_size - image_size) / 2u);
}

/* 在 SSD1673 帧缓冲中绘制资源字库里的“郑”字点阵。 */
static void epd_draw_zheng_glyph(int16_t x, int16_t y)
{
    AppGlyphInfo info;
    uint16_t py;

    if (!app_resources_begin_zheng_glyph(&info)) {
        return;
    }
    for (py = 0; py < info.height; py++) {
        if (!app_resources_read_glyph_row(g_resource_row_buf, info.stride)) {
            return;
        }
        epd_draw_1bpp_row(info.width, g_resource_row_buf, py, x, y, 1);
    }
}

/* 在 SSD1673 帧缓冲中绘制沙漏图片下方的周期和 TMP 平均温度文字。 */
static void epd_draw_hourglass_caption(uint16_t x, uint16_t y)
{
    char line[16];
    char *p;

    p = line;
    p = append_u16(p, app_hourglass_seconds());
    (void)append_str(p, "s AVG");
    epd_draw_string(x, y, line, 1);

    p = line;
    p = append_t10(p, g_tmp_avg_display_t10);
    (void)append_str(p, "C");
    epd_draw_string((uint16_t)(x + 5u), (uint16_t)(y + 10u), line, 1);
}

/* 在 SSD1673 主界面右侧加载沙漏图片关键帧并绘制平均温度说明。 */
static void epd_draw_hourglass(void)
{
    AppImageInfo info;
    int16_t x;
    int16_t y;
    uint8_t frame_index;

    x = EPD_HOURGLASS_X;
    y = EPD_HOURGLASS_Y;
    if (!app_resources_get_image_info(APP_RESOURCE_IMAGE_HOURGLASS, &info)) {
        return;
    }
    x = info.default_x;
    y = info.default_y;
    frame_index = epd_hourglass_asset_frame_index(info.frame_count);
    if (app_resources_begin_image_frame(APP_RESOURCE_IMAGE_HOURGLASS, frame_index)) {
        (void)epd_draw_image_rows(&info, x, y);
    }
    epd_draw_hourglass_caption((uint16_t)(x + 4), (uint16_t)(y + 68));
}

/* 在 GIF 页面左上角标记当前帧来源或失败状态。 */
static void epd_draw_gif_source_tag(const char *label)
{
    epd_fill_rect(0, 0, 36, 10, 0);
    epd_draw_string(2, 2, label, 1);
}

/* GIF 资源不可用时显示明确状态，避免误以为还在正常播放。 */
static void epd_draw_gif_missing(void)
{
    epd_draw_gif_source_tag("NO SD");
    epd_draw_string(86, 48, "NO SD", 2);
    epd_draw_string(72, 72, "MASCOT.BIN", 1);
}

/* 按行读取 GIF 资源帧并贴到 SSD1673 帧缓冲，避免占用整帧 RAM 缓存。 */
static uint8_t epd_draw_gif_resource_frame(const AppImageInfo *info)
{
    int16_t x;
    int16_t y;
    uint8_t scale;

    scale = info->default_scale;
    if (scale == 0) {
        scale = 1;
    }
    x = epd_center_coord(info->width, EPD_SCREEN_W, scale);
    y = epd_center_coord(info->height, EPD_SCREEN_H, scale);

    if (!epd_draw_image_rows(info, x, y)) {
        return 0;
    }
    epd_draw_gif_source_tag("SD");
    return 1;
}

/* 在 SSD1673 帧缓冲中居中绘制 GIF 转换动画当前帧。 */
static void epd_draw_gif_frame(void)
{
    AppImageInfo info;
    uint8_t index;

    if (app_resources_get_image_info(g_gif_image_id, &info)) {
        index = epd_next_frame_index(info.frame_count, &g_mascot_frame_index);
        if (app_resources_begin_image_frame(g_gif_image_id, index) &&
            epd_draw_gif_resource_frame(&info)) {
            return;
        }
    }

    epd_clear_buffer();
    epd_draw_gif_missing();
}

/* 使用 5x7 字库在 SSD1673 帧缓冲中绘制一个字符。 */
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

/* 使用 5x7 字库在 SSD1673 帧缓冲中绘制字符串。 */
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

/* 把 SSD1673 帧缓冲以局部刷新方式提交到屏幕。 */
static void epd_flush_partial(void)
{
    (void)epd_write_buffer_to_screen_partial(epd_buf);
}

/* 清空备用驱动渲染帧缓冲，0xFF 表示白色像素。 */
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

    g_epd_has_render_tick = 0;
    epd_request_render(1);
}

/* 在备用驱动逻辑坐标中写入一个像素。 */
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

/* 在备用驱动帧缓冲中填充一个小矩形，用于局部清出调试标识背景。 */
static void epd_alt_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t black)
{
    uint16_t px;
    uint16_t py;

    for (py = y; py < (uint16_t)(y + h) && py < EPD_ALT_SCREEN_H; py++) {
        for (px = x; px < (uint16_t)(x + w) && px < EPD_ALT_SCREEN_W; px++) {
            epd_alt_pixel(px, py, black);
        }
    }
}

/* 把一行 1bpp 数据贴到备用驱动帧缓冲。 */
static void epd_alt_draw_1bpp_row(uint16_t width, const uint8_t *row, uint16_t py,
                                  int16_t x, int16_t y, uint8_t scale)
{
    uint16_t px;
    uint8_t bit;
    uint8_t sx;
    uint8_t sy;

    if (scale == 0) {
        scale = 1;
    }

    for (px = 0; px < width; px++) {
        bit = (uint8_t)(0x80u >> (px & 7u));
        if ((row[px >> 3] & bit) == 0) {
            continue;
        }
        for (sy = 0; sy < scale; sy++) {
            for (sx = 0; sx < scale; sx++) {
                epd_alt_pixel((uint16_t)(x + (int16_t)(px * scale + sx)),
                              (uint16_t)(y + (int16_t)(py * scale + sy)), 1);
            }
        }
    }
}

/* 在备用驱动帧缓冲中绘制 24x24 的“郑”字点阵。 */
static void epd_alt_draw_zheng_glyph(int16_t x, int16_t y)
{
    AppGlyphInfo info;
    uint16_t py;

    if (!app_resources_begin_zheng_glyph(&info)) {
        return;
    }
    for (py = 0; py < info.height; py++) {
        if (!app_resources_read_glyph_row(g_resource_row_buf, info.stride)) {
            return;
        }
        epd_alt_draw_1bpp_row(info.width, g_resource_row_buf, py, x, y, 1);
    }
}

/* 在备用驱动帧缓冲中绘制沙漏图片下方的周期和 TMP 平均温度文字。 */
static void epd_alt_draw_hourglass_caption(uint16_t x, uint16_t y)
{
    char line[16];
    char *p;

    p = line;
    p = append_u16(p, app_hourglass_seconds());
    (void)append_str(p, "s AVG");
    epd_alt_draw_string(x, y, line, 1);

    p = line;
    p = append_t10(p, g_tmp_avg_display_t10);
    (void)append_str(p, "C");
    epd_alt_draw_string((uint16_t)(x + 5u), (uint16_t)(y + 10u), line, 1);
}

/* 从当前图片资源按行读取并贴到备用驱动帧缓冲。 */
static uint8_t epd_alt_draw_image_rows(const AppImageInfo *info, int16_t x, int16_t y)
{
    uint16_t py;
    uint8_t scale;

    scale = info->default_scale;
    if (scale == 0) {
        scale = 1;
    }

    for (py = 0; py < info->height; py++) {
        if (!app_resources_read_image_row(g_resource_row_buf, info->stride)) {
            return 0;
        }
        epd_alt_draw_1bpp_row(info->width, g_resource_row_buf, py, x, y, scale);
    }
    return 1;
}

/* 在备用驱动主界面右侧加载沙漏图片关键帧并绘制平均温度说明。 */
static void epd_alt_draw_hourglass(void)
{
    AppImageInfo info;
    int16_t x;
    int16_t y;
    uint8_t frame_index;

    x = EPD_ALT_HOURGLASS_X;
    y = EPD_ALT_HOURGLASS_Y;
    if (!app_resources_get_image_info(APP_RESOURCE_IMAGE_HOURGLASS, &info)) {
        return;
    }
    frame_index = epd_hourglass_asset_frame_index(info.frame_count);
    if (app_resources_begin_image_frame(APP_RESOURCE_IMAGE_HOURGLASS, frame_index)) {
        (void)epd_alt_draw_image_rows(&info, x, y);
    }
    epd_alt_draw_hourglass_caption((uint16_t)(x + 4), (uint16_t)(y + 68));
}

/* 在备用 GIF 页面左上角标记当前帧来源或失败状态。 */
static void epd_alt_draw_gif_source_tag(const char *label)
{
    epd_alt_fill_rect(0, 0, 36, 10, 0);
    epd_alt_draw_string(2, 2, label, 1);
}

/* 备用驱动下的 GIF 资源缺失提示。 */
static void epd_alt_draw_gif_missing(void)
{
    epd_alt_draw_gif_source_tag("NO SD");
    epd_alt_draw_string(48, 56, "NO SD", 2);
    epd_alt_draw_string(36, 80, "MASCOT.BIN", 1);
}

/* 按行读取 GIF 资源帧并贴到备用驱动帧缓冲。 */
static uint8_t epd_alt_draw_gif_resource_frame(const AppImageInfo *info)
{
    int16_t x;
    int16_t y;
    uint8_t scale;

    scale = info->default_scale;
    if (scale == 0) {
        scale = 1;
    }
    x = epd_center_coord(info->width, EPD_ALT_SCREEN_W, scale);
    y = epd_center_coord(info->height, EPD_ALT_SCREEN_H, scale);

    if (!epd_alt_draw_image_rows(info, x, y)) {
        return 0;
    }
    epd_alt_draw_gif_source_tag("SD");
    return 1;
}

/* 使用备用驱动渲染全屏 GIF 动画当前帧。 */
static void epd_alt_show_gif(void)
{
    AppImageInfo info;
    uint8_t index;

    epd_alt_clear_buffer();
    if (app_resources_get_image_info(g_gif_image_id, &info)) {
        index = epd_next_frame_index(info.frame_count, &g_mascot_frame_index);
        if (app_resources_begin_image_frame(g_gif_image_id, index) &&
            epd_alt_draw_gif_resource_frame(&info)) {
            (void)epd_alt_write_buffer_to_screen(epd_buf);
            return;
        }
    }

    epd_alt_clear_buffer();
    epd_alt_draw_gif_missing();
    (void)epd_alt_write_buffer_to_screen(epd_buf);
}

/* 使用 5x7 字库在备用驱动帧缓冲中绘制一个字符。 */
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

/* 使用 5x7 字库在备用驱动帧缓冲中绘制字符串。 */
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

/* 在 SSD1673 帧缓冲中绘制一个阅读页码点。 */
static void epd_draw_text_item(const TextReaderItem *item)
{
    AppGlyphInfo info;
    uint16_t py;

    if (item->codepoint < 128u) {
        epd_draw_char(item->x, item->y, (char)item->codepoint, 1);
        return;
    }
    if (!app_resources_begin_text_glyph(item->codepoint, &info)) {
        epd_draw_char(item->x, item->y, '?', 1);
        return;
    }
    for (py = 0; py < info.height; py++) {
        if (!app_resources_read_glyph_row(g_resource_row_buf, info.stride)) {
            return;
        }
        epd_draw_1bpp_row(info.width, g_resource_row_buf, py, item->x, item->y, 1);
    }
}

/* 在备用驱动帧缓冲中绘制一个阅读页码点。 */
static void epd_alt_draw_text_item(const TextReaderItem *item)
{
    AppGlyphInfo info;
    uint16_t py;

    if (item->codepoint < 128u) {
        epd_alt_draw_char(item->x, item->y, (char)item->codepoint, 1);
        return;
    }
    if (!app_resources_begin_text_glyph(item->codepoint, &info)) {
        epd_alt_draw_char(item->x, item->y, '?', 1);
        return;
    }
    for (py = 0; py < info.height; py++) {
        if (!app_resources_read_glyph_row(g_resource_row_buf, info.stride)) {
            return;
        }
        epd_alt_draw_1bpp_row(info.width, g_resource_row_buf, py, item->x, item->y, 1);
    }
}

/* 使用 SSD1673 渲染文本阅读页。 */
static void epd_show_text_page(void)
{
    TextReaderPage page;
    uint8_t i;

    epd_clear_buffer();
    epd_draw_string(0, 0, "BOOK", 1);
    if (!text_reader_build_page(EPD_SCREEN_W, EPD_TEXT_TOP_Y, EPD_TEXT_LINES, &page)) {
        epd_draw_string(54, 44, "NO BOOK", 2);
        epd_draw_string(42, 68, "TEXT/BOOK.TXT", 1);
        epd_flush_partial();
        return;
    }
    for (i = 0; i < page.count; i++) {
        epd_draw_text_item(&page.items[i]);
    }
    if (page.end) {
        epd_draw_string(220, 0, "END", 1);
    }
    epd_flush_partial();
}

/* 使用备用驱动渲染文本阅读页。 */
static void epd_alt_show_text_page(void)
{
    TextReaderPage page;
    uint8_t i;

    epd_alt_clear_buffer();
    epd_alt_draw_string(0, 0, "BOOK", 1);
    if (!text_reader_build_page(EPD_ALT_SCREEN_W, EPD_TEXT_ALT_TOP_Y, EPD_TEXT_ALT_LINES, &page)) {
        epd_alt_draw_string(30, 52, "NO BOOK", 2);
        epd_alt_draw_string(18, 76, "TEXT/BOOK.TXT", 1);
        (void)epd_alt_write_buffer_to_screen(epd_buf);
        return;
    }
    for (i = 0; i < page.count; i++) {
        epd_alt_draw_text_item(&page.items[i]);
    }
    if (page.end) {
        epd_alt_draw_string(150, 0, "END", 1);
    }
    (void)epd_alt_write_buffer_to_screen(epd_buf);
}

/* 将温度样本转换为主界面显示用的四行文本。 */
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

/* 将一个设置项转换为设置页面显示文本。 */
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
    case 3:
        p = append_str(p, "ALM TIME ");
        p = append_u16(p, app_alarm_duration_seconds());
        (void)append_str(p, "s");
        break;
    default:
        p = append_str(p, "HOURGLASS ");
        p = append_u16(p, app_hourglass_seconds());
        (void)append_str(p, "s");
        break;
    }
}

/* 使用备用驱动渲染设置页面并刷新到屏幕。 */
static void epd_alt_show_settings(void)
{
    uint8_t i;
    char line[28];

    epd_alt_clear_buffer();
    epd_alt_draw_string(0, 0, "SETTINGS", 2);
    for (i = 0; i < EPD_SETTINGS_ROWS; i++) {
        setting_to_line(i, (uint8_t)(i == g_epd_settings_selected), g_epd_settings_editing, line);
        epd_alt_draw_string(0, (uint16_t)(24u + i * 18u), line, 1);
    }
    (void)epd_alt_write_buffer_to_screen(epd_buf);
}

/* 使用 SSD1673 渲染设置页面并局部刷新到屏幕。 */
static void epd_show_settings(void)
{
    uint8_t i;
    char line[28];

    epd_clear_buffer();
    epd_draw_string(0, 0, "SETTINGS", 2);
    for (i = 0; i < EPD_SETTINGS_ROWS; i++) {
        setting_to_line(i, (uint8_t)(i == g_epd_settings_selected), g_epd_settings_editing, line);
        epd_draw_string(0, (uint16_t)(24u + i * 19u), line, 2);
    }
    epd_flush_partial();
}

/* 使用备用驱动渲染当前温度主界面。 */
static void epd_alt_show_current(const TempSample *s)
{
    char lines[4][24];
    uint8_t i;

    sample_to_lines(s, lines);
    epd_alt_clear_buffer();
    epd_alt_draw_string(0, 0, "TEMP LOGGER", 2);
    for (i = 0; i < 4u; i++) {
        epd_alt_draw_string(0, (uint16_t)(22u + i * 18u), lines[i], 2);
    }
    epd_alt_draw_hourglass();
    epd_alt_draw_zheng_glyph(EPD_ALT_ZHENG_X, EPD_ALT_ZHENG_Y);
    if (sample_over_threshold(s)) {
        epd_alt_draw_string(92, 124, "ALM 1", 2);
    } else {
        epd_alt_draw_string(92, 124, "ALM 0", 2);
    }
    (void)epd_alt_write_buffer_to_screen(epd_buf);
}

/* 使用 SSD1673 渲染当前温度主界面。 */
static void epd_show_current(const TempSample *s)
{
    char lines[4][24];
    uint8_t i;

    sample_to_lines(s, lines);
    epd_clear_buffer();
    epd_draw_string(0, 0, "TEMP LOGGER", 2);
    for (i = 0; i < 4u; i++) {
        epd_draw_string(0, (uint16_t)(22u + i * 18u), lines[i], 2);
    }
    if (sample_over_threshold(s)) {
        epd_draw_string(154, 104, "ALM 1", 2);
    } else {
        epd_draw_string(154, 104, "ALM 0", 2);
    }
    epd_draw_hourglass();
    epd_draw_zheng_glyph(EPD_ZHENG_X, EPD_ZHENG_Y);
    epd_flush_partial();
}

/* 使用 SSD1673 渲染全屏 GIF 动画当前帧。 */
static void epd_show_gif(void)
{
    epd_clear_buffer();
    epd_draw_gif_frame();
    epd_flush_partial();
}

void epd_force_next_current_refresh(void)
{
    g_epd_history_playback = 0;
    g_epd_gif_playback = 0;
    g_epd_render_view = EPD_VIEW_CURRENT;
    epd_request_render(1);
}

void epd_show_current_auto(const TempSample *s)
{
    if (!g_epd_auto_update) {
        return;
    }

    g_epd_target_sample = *s;
    g_epd_has_target_sample = 1;
    g_epd_render_view = EPD_VIEW_CURRENT;
    g_epd_gif_playback = 0;
    epd_hourglass_add_sample(s);
    epd_request_render(1);
}

/* 将 Flash 历史记录转换成一行简短文本。 */
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

/* 根据历史页起点和行号计算实际记录下标，支持循环滚动。 */
static uint16_t history_row_index(uint16_t start, uint8_t row, uint16_t count)
{
    uint16_t index;

    index = (uint16_t)(start + row);
    if (index >= count && count > HISTORY_ROWS_ON_EPD) {
        index = (uint16_t)(index - count);
    }
    return index;
}

/* 生成历史页面顶部的记录位置和自动播放状态文本。 */
static void history_meta_line(uint16_t start, uint16_t count, char *line)
{
    char *p;

    p = line;
    if (count == 0) {
        (void)append_str(p, "0 REC");
        return;
    }
    p = append_u16(p, (uint16_t)(start + 1u));
    *p++ = '/';
    p = append_u16(p, count);
    (void)append_str(p, " AUTO");
}

/* 使用备用驱动渲染历史记录页面。 */
static void epd_alt_show_history(uint16_t start)
{
    uint16_t count;
    uint16_t index;
    uint8_t row;
    TempRecord r;
    char line[40];

    count = history_count();
    if (start >= count && count > 0) {
        start = 0;
    }
    if (count <= HISTORY_ROWS_ON_EPD) {
        start = 0;
    }

    epd_alt_clear_buffer();
    epd_alt_draw_string(0, 0, "FLASH LOG", 2);
    history_meta_line(start, count, line);
    epd_alt_draw_string(0, 22, line, 1);

    if (count == 0) {
        epd_alt_draw_string(0, 44, "NO RECORD", 2);
        (void)epd_alt_write_buffer_to_screen(epd_buf);
        return;
    }

    for (row = 0; row < HISTORY_ROWS_ON_EPD; row++) {
        index = history_row_index(start, row, count);
        if (index < count && history_get(index, &r)) {
            record_to_line(&r, line);
            epd_alt_draw_string(0, (uint16_t)(40u + row * 18u), line, 1);
        }
    }

    (void)epd_alt_write_buffer_to_screen(epd_buf);
}

/* 使用 SSD1673 渲染历史记录页面。 */
static void epd_show_history(uint16_t start)
{
    uint16_t count;
    uint16_t index;
    uint8_t row;
    TempRecord r;
    char line[40];

    count = history_count();
    if (start >= count && count > 0) {
        start = 0;
    }
    if (count <= HISTORY_ROWS_ON_EPD) {
        start = 0;
    }

    epd_clear_buffer();
    epd_draw_string(0, 0, "FLASH LOG", 2);
    history_meta_line(start, count, line);
    epd_draw_string(132, 4, line, 1);

    if (count == 0) {
        epd_draw_string(0, 44, "NO RECORD", 2);
        epd_flush_partial();
        return;
    }

    for (row = 0; row < HISTORY_ROWS_ON_EPD; row++) {
        index = history_row_index(start, row, count);
        if (index < count && history_get(index, &r)) {
            record_to_line(&r, line);
            epd_draw_string(0, (uint16_t)(28u + row * 18u), line, 1);
        }
    }

    epd_flush_partial();
}

void epd_show_history_page(uint16_t start)
{
    g_epd_target_history_start = start;
    g_epd_auto_update = 0;
    g_epd_history_playback = 0;
    g_epd_gif_playback = 0;
    g_epd_render_view = EPD_VIEW_HISTORY;
    epd_request_render(1);
}

void epd_show_history_playback(void)
{
    uint16_t count;

    count = history_count();
    g_epd_target_history_start = epd_history_latest_start(count);
    g_epd_auto_update = 0;
    g_epd_history_playback = 1;
    g_epd_gif_playback = 0;
    g_epd_render_view = EPD_VIEW_HISTORY;
    g_epd_last_history_scroll_tick = board_tick10();
    epd_request_render(1);
}

void epd_show_gif_playback(void)
{
    g_epd_auto_update = 0;
    g_epd_history_playback = 0;
    g_epd_gif_playback = 1;
    g_epd_render_view = EPD_VIEW_GIF;
    g_mascot_frame_index = 0;
    g_gif_image_id = app_resources_first_gif_image();
    if (g_gif_image_id == 0) {
        g_gif_image_id = APP_RESOURCE_IMAGE_MASCOT;
    }
    g_epd_last_gif_frame_tick = board_tick10();
    epd_request_render(1);
}

void epd_gif_prev_asset(void)
{
    uint8_t next_id;

    if (g_epd_render_view != EPD_VIEW_GIF) {
        epd_show_gif_playback();
        return;
    }
    next_id = app_resources_step_gif_image(g_gif_image_id, -1);
    if (next_id != 0) {
        g_gif_image_id = next_id;
        g_mascot_frame_index = 0;
        epd_request_render(1);
    }
}

void epd_gif_next_asset(void)
{
    uint8_t next_id;

    if (g_epd_render_view != EPD_VIEW_GIF) {
        epd_show_gif_playback();
        return;
    }
    next_id = app_resources_step_gif_image(g_gif_image_id, 1);
    if (next_id != 0) {
        g_gif_image_id = next_id;
        g_mascot_frame_index = 0;
        epd_request_render(1);
    }
}

void epd_show_text_reader(void)
{
    g_epd_auto_update = 0;
    g_epd_history_playback = 0;
    g_epd_gif_playback = 0;
    g_epd_render_view = EPD_VIEW_TEXT;
    text_reader_reset();
    epd_request_render(1);
}

void epd_text_prev_page(void)
{
    if (g_epd_render_view != EPD_VIEW_TEXT) {
        epd_show_text_reader();
        return;
    }
    text_reader_prev_page();
    epd_request_render(1);
}

void epd_text_next_page(void)
{
    if (g_epd_render_view != EPD_VIEW_TEXT) {
        epd_show_text_reader();
        return;
    }
    text_reader_next_page();
    epd_request_render(1);
}

void epd_show_settings_page(uint8_t selected, uint8_t editing)
{
    if (selected >= EPD_SETTINGS_ROWS) {
        selected = (uint8_t)(EPD_SETTINGS_ROWS - 1u);
    }
    g_epd_settings_selected = selected;
    g_epd_settings_editing = editing ? 1u : 0u;
    g_epd_auto_update = 0;
    g_epd_history_playback = 0;
    g_epd_gif_playback = 0;
    g_epd_render_view = EPD_VIEW_SETTINGS;
    epd_request_render(1);
}

void epd_render_task(void)
{
    uint8_t rendered;

    if (!g_epd_render_dirty) {
        if (epd_gif_frame_due()) {
            g_epd_render_dirty = 1;
        } else if (epd_history_frame_due()) {
            g_epd_render_dirty = 1;
        } else if (epd_auto_frame_due()) {
            g_epd_render_dirty = 1;
        } else {
            return;
        }
    }

    if (!epd_render_frame_ready()) {
        return;
    }

    g_epd_render_dirty = 0;
    rendered = 0;

    if (g_epd_render_view == EPD_VIEW_GIF) {
        if (g_epd_driver == EPD_DRIVER_SPD2701) {
            epd_alt_show_gif();
        } else {
            epd_show_gif();
        }
        g_epd_last_gif_frame_tick = board_tick10();
        rendered = 1;
    } else if (g_epd_render_view == EPD_VIEW_HISTORY) {
        if (g_epd_driver == EPD_DRIVER_SPD2701) {
            epd_alt_show_history(g_epd_target_history_start);
        } else {
            epd_show_history(g_epd_target_history_start);
        }
        rendered = 1;
    } else if (g_epd_render_view == EPD_VIEW_SETTINGS) {
        if (g_epd_driver == EPD_DRIVER_SPD2701) {
            epd_alt_show_settings();
        } else {
            epd_show_settings();
        }
        rendered = 1;
    } else if (g_epd_render_view == EPD_VIEW_TEXT) {
        if (g_epd_driver == EPD_DRIVER_SPD2701) {
            epd_alt_show_text_page();
        } else {
            epd_show_text_page();
        }
        rendered = 1;
    } else if (g_epd_has_target_sample) {
        if (g_epd_driver == EPD_DRIVER_SPD2701) {
            epd_alt_show_current(&g_epd_target_sample);
        } else {
            epd_show_current(&g_epd_target_sample);
        }
        if (g_epd_auto_update) {
            g_epd_last_auto_frame_tick = board_tick10();
        }
        rendered = 1;
    }

    if (rendered) {
        epd_note_render_frame();
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
    g_epd_history_playback = 0;
    g_epd_gif_playback = 0;
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
    g_epd_history_playback = 0;
    g_epd_gif_playback = 0;
    epd_alt_init();
    epd_force_next_current_refresh();
}
