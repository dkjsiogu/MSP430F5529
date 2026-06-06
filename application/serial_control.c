#include "serial_control.h"

#include "app_config.h"
#include "app_resources.h"
#include "app_state.h"
#include "board.h"
#include "epaper.h"
#include "flash_log.h"
#include "sensors.h"
#include "uart.h"

static uint16_t g_history_page = 0;
static SerialFlashEraseHandler g_flash_erase_handler = 0;

static void serial_control_erase_flash_log(void)
{
    if (g_flash_erase_handler != 0) {
        g_flash_erase_handler();
    } else {
        flash_erase_log();
        epd_show_history_page(0);
    }
}

void serial_control_set_flash_erase_handler(SerialFlashEraseHandler handler)
{
    g_flash_erase_handler = handler;
}

/* 瑙ｆ瀽涓插彛鏀跺埌鐨勫崟瀛楃鍛戒护锛屽苟鏄犲皠涓烘湰鍦版樉绀烘垨璁剧疆鍔ㄤ綔銆?*/
static void handle_rx_char(uint8_t cmd)
{
    uint16_t count;

    if (cmd >= 'A' && cmd <= 'Z') {
        cmd = (uint8_t)(cmd + ('a' - 'A'));
    }

    switch (cmd) {
    case 'h':
        g_history_page = 0;
        epd_show_history_page(0);
        break;
    case 'n':
        count = history_count();
        if (count == 0) {
            g_history_page = 0;
            epd_show_history_page(0);
            break;
        }
        g_history_page = (uint16_t)(g_history_page + HISTORY_PAGE_ROWS);
        if (g_history_page >= count) {
            g_history_page = 0;
        }
        epd_show_history_page(g_history_page);
        break;
    case 'c':
        epd_force_next_current_refresh();
        sample_timer_force_due();
        break;
    case '+':
        (void)app_adjust_threshold_t10(ALERT_THRESHOLD_STEP_T10);
        epd_force_next_current_refresh();
        sample_timer_force_due();
        break;
    case '-':
        (void)app_adjust_threshold_t10(-ALERT_THRESHOLD_STEP_T10);
        epd_force_next_current_refresh();
        sample_timer_force_due();
        break;
    case 'e':
        g_history_page = 0;
        serial_control_erase_flash_log();
        break;
    case 'i':
        (void)tmp421_detect();
        epd_force_next_current_refresh();
        sample_timer_force_due();
        break;
    case 'm':
        epd_use_alt_auto();
        sample_timer_force_due();
        break;
    case 'r':
        epd_resume_auto();
        sample_timer_force_due();
        break;
    case 'w':
        (void)app_resources_write_probe();
        epd_force_next_current_refresh();
        sample_timer_force_due();
        break;
    default:
        break;
    }
}

void serial_control_poll(void)
{
    uint8_t cmd;

    cmd = uart_take_rx();
    if (cmd != 0) {
        handle_rx_char(cmd);
    }
}
