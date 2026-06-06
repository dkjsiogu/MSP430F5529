#include "epaper.h"
#include "app_config.h"
#include "app_state.h"
#include "board.h"
#include "flash_log.h"
#include "format.h"
#include "app_resources.h"
#include "text_reader.h"
#include "display_config.h"
#include "epd_panel_msp430.h"

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

#define EPD_VIEW_CURRENT            0u /* 褰撳墠娓╁害涓荤晫闈㈡覆鏌撶洰鏍囥€?*/
#define EPD_VIEW_HISTORY            1u /* Flash 鍘嗗彶璁板綍鐣岄潰娓叉煋鐩爣銆?*/
#define EPD_VIEW_SETTINGS           2u /* 璁剧疆鐣岄潰娓叉煋鐩爣銆?*/
#define EPD_VIEW_GIF                3u /* 鍏ㄥ睆 GIF 鍔ㄧ敾鎾斁鐣岄潰娓叉煋鐩爣銆?*/
#define EPD_VIEW_TEXT               4u /* 鏂囨湰闃呰鐣岄潰娓叉煋鐩爣銆?*/
#define EPD_SETTINGS_ROWS           5u /* 璁剧疆椤甸潰鏄剧ず鐨勯厤缃」鏁伴噺銆?*/
#define EPD_HOURGLASS_X             198 /* SSD1673 涓荤晫闈㈡矙婕忛粯璁?X 鍧愭爣锛岃祫婧愮储寮曠己澶辨椂浣跨敤銆?*/
#define EPD_HOURGLASS_Y             12  /* SSD1673 涓荤晫闈㈡矙婕忛粯璁?Y 鍧愭爣锛岃祫婧愮储寮曠己澶辨椂浣跨敤銆?*/
#define EPD_ALT_HOURGLASS_X         120 /* 澶囩敤椹卞姩涓荤晫闈㈡矙婕?X 鍧愭爣銆?*/
#define EPD_ALT_HOURGLASS_Y         4   /* 澶囩敤椹卞姩涓荤晫闈㈡矙婕?Y 鍧愭爣銆?*/
#define EPD_ZHENG_X                 170 /* SSD1673 涓荤晫闈⑩€滈儜鈥濆瓧缁樺埗 X 鍧愭爣锛屼綅浜庢矙婕忓乏渚с€?*/
#define EPD_ZHENG_Y                 38  /* SSD1673 涓荤晫闈⑩€滈儜鈥濆瓧缁樺埗 Y 鍧愭爣銆?*/
#define EPD_ALT_ZHENG_X             94  /* 澶囩敤椹卞姩涓荤晫闈⑩€滈儜鈥濆瓧缁樺埗 X 鍧愭爣銆?*/
#define EPD_ALT_ZHENG_Y             96  /* 澶囩敤椹卞姩涓荤晫闈⑩€滈儜鈥濆瓧缁樺埗 Y 鍧愭爣銆?*/
#define EPD_TEXT_TOP_Y              14u /* SSD1673 闃呰椤垫鏂囪捣濮?Y 鍧愭爣銆?*/
#define EPD_TEXT_ALT_TOP_Y          12u /* 澶囩敤椹卞姩闃呰椤垫鏂囪捣濮?Y 鍧愭爣銆?*/
#define EPD_TEXT_LINES              6u  /* SSD1673 闃呰椤垫鏂囪鏁般€?*/
#define EPD_TEXT_ALT_LINES          7u  /* 澶囩敤椹卞姩闃呰椤垫鏂囪鏁般€?*/
#define EPD_UI_GLYPH_W              16u /* 设置页内置中文字形宽度，单位像素。 */
#define EPD_UI_GLYPH_H              16u /* 设置页内置中文字形高度，单位像素。 */
#define EPD_UI_GLYPH_STRIDE         2u  /* 设置页内置中文字形每行字节数。 */
#define EPD_UI_GLYPH_BYTES          (EPD_UI_GLYPH_H * EPD_UI_GLYPH_STRIDE) /* 单个内置中文字形字节数。 */
#define EPD_ARRAY_COUNT(a)          (sizeof(a) / sizeof((a)[0])) /* 计算静态数组元素数量。 */

typedef struct {
    uint16_t codepoint;                                  /* Unicode 码点。 */
    uint8_t bitmap[EPD_UI_GLYPH_BYTES];                  /* 16x16 1bpp 点阵，bit=1 表示黑点。 */
} EpdUiGlyph;

typedef struct {
    const uint16_t *label;                               /* 设置项中文标签。 */
    uint8_t label_len;                                   /* 标签字数。 */
    uint16_t unit;                                       /* 设置值后缀单位，0 表示无单位。 */
} EpdSettingRow;

static const EpdUiGlyph epd_ui_glyphs[] = {
    {0x8BBEu, {0x43u,0xF8u,0x73u,0x08u,0x33u,0x08u,0x06u,0x0Cu,0x0Cu,0x0Fu,0xE0u,0x00u,0x67u,0xFCu,0x62u,0x0Cu,0x63u,0x08u,0x61u,0x98u,0x68u,0xF0u,0x78u,0xE0u,0x71u,0xF8u,0x4Fu,0x1Eu,0x0Cu,0x06u,0x00u,0x00u}},
    {0x7F6Eu, {0x00u,0x00u,0x7Fu,0xFCu,0x44u,0x44u,0x7Fu,0xFCu,0x01u,0x00u,0xFFu,0xFEu,0x01u,0x00u,0x3Fu,0xFCu,0x20u,0x0Cu,0x3Fu,0xFCu,0x20u,0x0Cu,0x3Fu,0xFCu,0x20u,0x0Cu,0x20u,0x0Cu,0xFFu,0xFFu,0x00u,0x00u}},
    {0x91C7u, {0xFFu,0xF8u,0x02u,0x04u,0x43u,0x0Cu,0x61u,0x98u,0x31u,0x90u,0x11u,0x30u,0x01u,0x00u,0xFFu,0xFFu,0x07u,0xC0u,0x05u,0x40u,0x19u,0x20u,0x31u,0x18u,0xE1u,0x0Eu,0x81u,0x02u,0x01u,0x00u,0x01u,0x00u}},
    {0x6837u, {0x21u,0x0Cu,0x21u,0x88u,0x20u,0x98u,0xFFu,0xFEu,0x60u,0x60u,0x60u,0x60u,0x70u,0x60u,0x7Bu,0xFEu,0xACu,0x60u,0xACu,0x60u,0xA0u,0x60u,0x27u,0xFFu,0x20u,0x60u,0x20u,0x60u,0x20u,0x60u,0x20u,0x60u}},
    {0x79D2u, {0x38u,0x20u,0xF0u,0x20u,0x20u,0x2Cu,0x21u,0x24u,0x21u,0x26u,0xFFu,0x22u,0x63u,0x22u,0x62u,0x20u,0x72u,0x26u,0xF8u,0x26u,0xACu,0x0Cu,0xA0u,0x18u,0x20u,0x30u,0x20u,0xE0u,0x23u,0x80u,0x2Eu,0x00u}},
    {0x62A5u, {0x20u,0x00u,0x27u,0xFEu,0x26u,0x06u,0xFEu,0x06u,0x66u,0x7Cu,0x26u,0x00u,0x27u,0xFEu,0x3Eu,0x86u,0xF6u,0xC4u,0xE6u,0x4Cu,0x26u,0x68u,0x26u,0x38u,0x26u,0x78u,0x66u,0xCEu,0xE7u,0x82u,0x00u,0x00u}},
    {0x8B66u, {0x24u,0x20u,0xFFu,0xA0u,0x60u,0x7Fu,0x7Fu,0xE4u,0x83u,0xBCu,0x7Au,0x18u,0x4Au,0x66u,0x7Eu,0x00u,0x01u,0x00u,0xFFu,0xFFu,0x00u,0x00u,0x3Fu,0xF8u,0x00u,0x00u,0x3Fu,0xF8u,0x20u,0x08u,0x3Fu,0xF8u}},
    {0x6E29u, {0xC0u,0x00u,0x67u,0xFCu,0x24u,0x0Cu,0x07u,0xFCu,0x84u,0x04u,0xC7u,0xFCu,0x64u,0x0Cu,0x44u,0x00u,0x0Fu,0xFEu,0x4Du,0xB6u,0x6Du,0xB6u,0x4Du,0xB6u,0xCDu,0xB6u,0xCDu,0xB6u,0xFFu,0xFFu,0x80u,0x00u}},
    {0x5EA6u, {0x01u,0x80u,0x01u,0x80u,0x7Fu,0xFFu,0x40u,0x00u,0x40u,0x10u,0x7Fu,0xFEu,0x46u,0x10u,0x47u,0xF0u,0x40u,0x00u,0x5Fu,0xFCu,0x46u,0x18u,0xC3u,0x30u,0xC1u,0xE0u,0x8Fu,0xF8u,0x38u,0x0Eu,0x00u,0x00u}},
    {0x8BB0u, {0xC0u,0x00u,0x67u,0xFEu,0x60u,0x06u,0x20u,0x06u,0x00u,0x06u,0x00u,0x06u,0xE0u,0x06u,0x63u,0xFEu,0x62u,0x06u,0x62u,0x00u,0x62u,0x00u,0x62u,0x03u,0x6Au,0x03u,0x7Au,0x03u,0x73u,0x06u,0x43u,0xFEu}},
    {0x5F55u, {0x7Fu,0xF8u,0x00u,0x08u,0x00u,0x08u,0x7Fu,0xF8u,0x00u,0x08u,0x00u,0x08u,0xFFu,0xFFu,0x01u,0x80u,0x71u,0x8Cu,0x11u,0xF8u,0x07u,0x60u,0x3Du,0x38u,0xF1u,0x0Eu,0x83u,0x02u,0x0Fu,0x00u,0x00u,0x00u}},
    {0x6761u, {0x18u,0x00u,0x1Fu,0xF8u,0x38u,0x18u,0x6Cu,0x30u,0xC7u,0xE0u,0x03u,0xC0u,0x3Eu,0xFCu,0xE1u,0x0Eu,0x01u,0x00u,0x7Fu,0xFCu,0x01u,0x80u,0x11u,0x20u,0x31u,0x38u,0x61u,0x0Eu,0x8Fu,0x02u,0x00u,0x00u}},
    {0x6570u, {0x18u,0x20u,0xDBu,0x20u,0x5Au,0x60u,0x18u,0x7Fu,0xFFu,0x46u,0x38u,0xC6u,0x7Eu,0xC4u,0x9Bu,0xE4u,0x30u,0x6Cu,0xFEu,0x2Cu,0x62u,0x38u,0xC6u,0x18u,0x3Cu,0x38u,0x3Cu,0x6Cu,0xE2u,0xC6u,0x81u,0x83u}},
    {0x65F6u, {0x00u,0x08u,0x00u,0x08u,0xFCu,0x08u,0x8Cu,0x08u,0x8Fu,0xFFu,0x8Cu,0x0Cu,0x8Cu,0x08u,0xFDu,0x08u,0x8Du,0x88u,0x8Cu,0xC8u,0x8Cu,0xC8u,0x8Cu,0x08u,0xFCu,0x08u,0x8Cu,0x08u,0x8Cu,0xF8u,0x00u,0x00u}},
    {0x957Fu, {0x30u,0x08u,0x30u,0x38u,0x30u,0xE0u,0x33u,0x80u,0x36u,0x00u,0x30u,0x00u,0xFFu,0xFFu,0x31u,0x80u,0x31u,0x80u,0x30u,0xC0u,0x30u,0x40u,0x30u,0x60u,0x33u,0x38u,0x3Fu,0x0Eu,0x3Cu,0x02u,0x30u,0x00u}},
    {0x6C99u, {0xC0u,0x40u,0x60u,0x40u,0x32u,0x40u,0x02u,0x48u,0x86u,0x4Cu,0xC4u,0x46u,0x6Cu,0x42u,0x08u,0x44u,0x20u,0x4Cu,0x60u,0x58u,0x60u,0x30u,0x40u,0x60u,0xC1u,0xC0u,0xCFu,0x00u,0x98u,0x00u,0x00u,0x00u}},
    {0x6F0Fu, {0x80u,0x00u,0xCFu,0xFEu,0x68u,0x06u,0x0Fu,0xFEu,0x88u,0x06u,0xC8u,0x00u,0xCFu,0xFFu,0x48u,0x60u,0x1Fu,0xFEu,0x5Eu,0x62u,0x5Fu,0x72u,0xDEu,0xEAu,0xD7u,0x72u,0xB6u,0xEAu,0xB6u,0x62u,0xA6u,0x7Eu}},
    {0x5468u, {0x7Fu,0xFEu,0x40u,0x06u,0x40u,0x02u,0x4Fu,0xF2u,0x41u,0x82u,0x41u,0x82u,0x5Fu,0xFAu,0x40u,0x02u,0x4Fu,0xE2u,0x4Cu,0x22u,0x4Cu,0x22u,0xCFu,0xE2u,0x8Cu,0x02u,0x80u,0x06u,0x00u,0x3Eu,0x00u,0x00u}},
    {0x671Fu, {0x46u,0x7Eu,0xFFu,0x46u,0x46u,0x42u,0x46u,0x42u,0x7Eu,0x7Eu,0x46u,0x42u,0x46u,0x42u,0x7Eu,0x42u,0x46u,0x7Eu,0x46u,0x42u,0xFFu,0x42u,0x6Cu,0xC2u,0x62u,0x86u,0xC1u,0x9Eu,0x81u,0x00u,0x00u,0x00u}},
    {0x9009u, {0x40u,0x60u,0x42u,0x60u,0x66u,0x60u,0x27u,0xFEu,0x04u,0x60u,0x0Cu,0x60u,0xE0u,0x60u,0x6Fu,0xFFu,0x61u,0x30u,0x61u,0x32u,0x63u,0x32u,0x66u,0x32u,0x6Cu,0x1Eu,0xB8u,0x00u,0x0Fu,0xFEu,0x00u,0x00u}},
    {0x6539u, {0x00u,0xC0u,0xFCu,0x80u,0x0Du,0xFFu,0x0Du,0x8Cu,0x0Du,0x0Cu,0x0Fu,0x88u,0xFFu,0x88u,0xC0u,0x98u,0xC0u,0xD8u,0xC4u,0x70u,0xCCu,0x70u,0xD8u,0x70u,0xE1u,0xD8u,0xC7u,0x0Eu,0x86u,0x02u,0x00u,0x00u}}
};

static const uint16_t epd_label_settings[] = {0x8BBEu, 0x7F6Eu};
static const uint16_t epd_label_sample[] = {0x91C7u, 0x6837u};
static const uint16_t epd_label_alarm_temp[] = {0x62A5u, 0x8B66u, 0x6E29u, 0x5EA6u};
static const uint16_t epd_label_storage[] = {0x8BB0u, 0x5F55u, 0x6761u, 0x6570u};
static const uint16_t epd_label_alarm_time[] = {0x62A5u, 0x8B66u, 0x65F6u, 0x957Fu};
static const uint16_t epd_label_hourglass[] = {0x6C99u, 0x6F0Fu, 0x5468u, 0x671Fu};

static const EpdSettingRow epd_setting_rows[] = {
    {epd_label_sample, (uint8_t)EPD_ARRAY_COUNT(epd_label_sample), 0x79D2u},
    {epd_label_alarm_temp, (uint8_t)EPD_ARRAY_COUNT(epd_label_alarm_temp), 0x5EA6u},
    {epd_label_storage, (uint8_t)EPD_ARRAY_COUNT(epd_label_storage), 0x6761u},
    {epd_label_alarm_time, (uint8_t)EPD_ARRAY_COUNT(epd_label_alarm_time), 0x79D2u},
    {epd_label_hourglass, (uint8_t)EPD_ARRAY_COUNT(epd_label_hourglass), 0x79D2u}
};

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

/* 鍓嶇疆澹版槑 SSD1673 瀛楃涓茬粯鍒跺嚱鏁帮紝渚涙矙婕忓瓧骞曟彁鍓嶈皟鐢ㄣ€?*/
static void epd_draw_string(uint16_t x, uint16_t y, const char *s, uint8_t scale);

/* 鍓嶇疆澹版槑澶囩敤椹卞姩瀛楃涓茬粯鍒跺嚱鏁帮紝渚涙矙婕忓瓧骞曟彁鍓嶈皟鐢ㄣ€?*/
static void epd_alt_draw_string(uint16_t x, uint16_t y, const char *s, uint8_t scale);

/* 璁＄畻褰撳墠娌欐紡鍛ㄦ湡瀵瑰簲鐨?10ms 鑺傛媿鏁般€?*/
static uint16_t epd_hourglass_period_ticks(void)
{
    return (uint16_t)((uint16_t)app_hourglass_seconds() * BOARD_TICKS_PER_SECOND);
}

/* 鏍规嵁褰撳墠绱鐨?TMP 鏍锋湰璁＄畻骞冲潎娓╁害銆?*/
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

/* 缁撴潫褰撳墠缁熻绐楀彛锛屽苟鎶婃渶鍚庝竴娆℃湁鏁堝钩鍧囧€间繚鐣欑粰鐣岄潰鏄剧ず銆?*/
static void epd_hourglass_reset_avg_window(void)
{
    if (g_tmp_avg_count > 0) {
        g_tmp_avg_display_t10 = epd_hourglass_current_avg_t10();
    }
    g_tmp_avg_sum_t10 = 0;
    g_tmp_avg_count = 0;
}

/* 鏍规嵁绯荤粺鏃堕棿鎺ㄨ繘娌欐紡鍛ㄦ湡锛屽懆鏈熺粨鏉熸椂鍚屾閲嶇疆 TMP 骞冲潎绐楀彛銆?*/
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

/* 鎶婃柊鐨?TMP 鏍锋湰鍔犲叆褰撳墠娌欐紡鍛ㄦ湡鐨勫钩鍧囨俯搴︾粺璁°€?*/
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

/* 鍒ゆ柇褰撳墠娌欐紡鏄惁澶勪簬鍛ㄦ湡鏈熬鐨勭炕杞樁娈碉紝骞惰繑鍥炵炕杞抚鍙枫€?*/
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

/* 鏍囪澧ㄦ按灞忔湁鏂扮殑鐩爣鐢婚潰闇€瑕佹覆鏌撱€?*/
static void epd_request_render(uint8_t urgent)
{
    (void)urgent;
    g_epd_render_dirty = 1;
}

/* 鍒ゆ柇褰撳墠鏄惁鍏佽鍚戝ⅷ姘村睆鎻愪氦涓嬩竴甯э紝鎸夐厤缃粺涓€闄愬埗灞忓箷甯х巼銆?*/
static uint8_t epd_render_frame_ready(void)
{
    if (!g_epd_has_render_tick) {
        return 1;
    }
    return board_tick10_elapsed(g_epd_last_render_tick, EPD_RENDER_MIN_TICKS);
}

/* 璁板綍鏈€杩戜竴娆″疄闄呭埛灞忔椂闂达紝璁╂寜閿搷搴斿拰澧ㄦ按灞忓抚鐜囪В鑰︺€?*/
static void epd_note_render_frame(void)
{
    g_epd_has_render_tick = 1;
    g_epd_last_render_tick = board_tick10();
}

/* 鍒ゆ柇涓荤晫闈㈣嚜鍔ㄥ姩鐢诲抚鏄惁鍒拌揪鍒锋柊闂撮殧銆?*/
static uint8_t epd_auto_frame_due(void)
{
    if (!g_epd_auto_update || !g_epd_has_target_sample) {
        return 0;
    }
    return board_tick10_elapsed(g_epd_last_auto_frame_tick, EPD_AUTO_FRAME_TICKS);
}

/* 鍒ゆ柇鍏ㄥ睆 GIF 鎾斁鐣岄潰鏄惁闇€瑕佹帹杩涘埌涓嬩竴甯с€?*/
static uint8_t epd_gif_frame_due(void)
{
    if (!g_epd_gif_playback || g_epd_render_view != EPD_VIEW_GIF) {
        return 0;
    }
    return board_tick10_elapsed(g_epd_last_gif_frame_tick, EPD_AUTO_FRAME_TICKS);
}

/* 鏍规嵁鍘嗗彶璁板綍鏁伴噺璁＄畻鑷姩鎾斁鏃朵紭鍏堟樉绀烘渶杩戣褰曠殑璧峰涓嬫爣銆?*/
static uint16_t epd_history_latest_start(uint16_t count)
{
    if (count <= HISTORY_PAGE_ROWS) {
        return 0;
    }
    return (uint16_t)(count - HISTORY_PAGE_ROWS);
}

/* 鍒ゆ柇鍘嗗彶璁板綍鑷姩鎾斁鏄惁闇€瑕佹粴鍔ㄥ埌涓嬩竴椤点€?*/
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
    if (count == 0 || count <= HISTORY_PAGE_ROWS) {
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

/* 调用底层 SSD1673 面板驱动初始化，渲染层不直接操作 GPIO/SPI。 */
static void epd_ssd_init_controller_and_clear(void)
{
    epd_panel_ssd_init_controller_and_clear();
}

/* 调用底层 SSD1673 局部刷新提交。 */
static uint8_t epd_write_buffer_to_screen_partial(const uint8_t *buf)
{
    return epd_panel_ssd_flush_partial(buf);
}

/* 调用底层备用面板刷新提交。 */
static uint8_t epd_alt_write_buffer_to_screen(const uint8_t *buf)
{
    return epd_panel_alt_flush(buf);
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

/* 鍒濆鍖栧鐢ㄥⅷ姘村睆鎺у埗鍣ㄣ€?*/
static void epd_alt_init(void)
{
    epd_panel_alt_init();
}

static void epd_clear_buffer(void)
{
    uint16_t i;

    for (i = 0; i < EPD_BUF_SIZE; i++) {
        epd_buf[i] = 0xFF;
    }
}

/* 鍦?SSD1673 閫昏緫鍧愭爣涓啓鍏ヤ竴涓儚绱犮€?*/
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

/* 鍦?SSD1673 甯х紦鍐蹭腑濉厖涓€涓皬鐭╁舰锛岀敤浜庡眬閮ㄦ竻鍑鸿皟璇曟爣璇嗚儗鏅€?*/
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

/* 鏍规嵁娌欐紡鍛ㄦ湡鏃堕棿閫夋嫨鍥剧墖鍏抽敭甯э紝鏈熬鎸夌炕杞椂闀挎槧灏勫埌鏈€鍚庡嚑甯с€?*/
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

/* 鎶婁竴琛?1bpp 鏁版嵁璐村埌 SSD1673 甯х紦鍐层€?*/
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

/* 浠庡綋鍓嶅浘鐗囪祫婧愭寜琛岃鍙栧苟璐村埌 SSD1673 甯х紦鍐层€?*/
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

/* 鎸夊抚鏁板惊鐜帹杩涘浘鐗囧抚娓告爣銆?*/
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

/* 璁＄畻鎸囧畾灏哄鍥剧墖鍦ㄥ睆骞曞崟鏂瑰悜涓婄殑灞呬腑鍧愭爣銆?*/
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

/* 鍦?SSD1673 甯х紦鍐蹭腑缁樺埗璧勬簮瀛楀簱閲岀殑鈥滈儜鈥濆瓧鐐归樀銆?*/
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

/* 鍦?SSD1673 甯х紦鍐蹭腑缁樺埗娌欐紡鍥剧墖涓嬫柟鐨勫懆鏈熷拰 TMP 骞冲潎娓╁害鏂囧瓧銆?*/
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

/* 鍦?SSD1673 涓荤晫闈㈠彸渚у姞杞芥矙婕忓浘鐗囧叧閿抚骞剁粯鍒跺钩鍧囨俯搴﹁鏄庛€?*/
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

/* 鍦?GIF 椤甸潰宸︿笂瑙掓爣璁板綋鍓嶅抚鏉ユ簮鎴栧け璐ョ姸鎬併€?*/
static void epd_draw_gif_source_tag(const char *label)
{
    epd_fill_rect(0, 0, 36, 10, 0);
    epd_draw_string(2, 2, label, 1);
}

/* GIF 璧勬簮涓嶅彲鐢ㄦ椂鏄剧ず鏄庣‘鐘舵€侊紝閬垮厤璇互涓鸿繕鍦ㄦ甯告挱鏀俱€?*/
static void epd_draw_gif_missing(void)
{
    epd_draw_gif_source_tag("NO SD");
    epd_draw_string(86, 48, "NO SD", 2);
    epd_draw_string(72, 72, "MASCOT.BIN", 1);
}

/* 鎸夎璇诲彇 GIF 璧勬簮甯у苟璐村埌 SSD1673 甯х紦鍐诧紝閬垮厤鍗犵敤鏁村抚 RAM 缂撳瓨銆?*/
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

/* 鍦?SSD1673 甯х紦鍐蹭腑灞呬腑缁樺埗 GIF 杞崲鍔ㄧ敾褰撳墠甯с€?*/
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

/* 浣跨敤 5x7 瀛楀簱鍦?SSD1673 甯х紦鍐蹭腑缁樺埗涓€涓瓧绗︺€?*/
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

/* 浣跨敤 5x7 瀛楀簱鍦?SSD1673 甯х紦鍐蹭腑缁樺埗瀛楃涓层€?*/
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

/* 鎶?SSD1673 甯х紦鍐蹭互灞€閮ㄥ埛鏂版柟寮忔彁浜ゅ埌灞忓箷銆?*/
static void epd_flush_partial(void)
{
    (void)epd_write_buffer_to_screen_partial(epd_buf);
}

/* 娓呯┖澶囩敤椹卞姩娓叉煋甯х紦鍐诧紝0xFF 琛ㄧず鐧借壊鍍忕礌銆?*/
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

/* 鍦ㄥ鐢ㄩ┍鍔ㄩ€昏緫鍧愭爣涓啓鍏ヤ竴涓儚绱犮€?*/
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

/* 鍦ㄥ鐢ㄩ┍鍔ㄥ抚缂撳啿涓～鍏呬竴涓皬鐭╁舰锛岀敤浜庡眬閮ㄦ竻鍑鸿皟璇曟爣璇嗚儗鏅€?*/
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

/* 鎶婁竴琛?1bpp 鏁版嵁璐村埌澶囩敤椹卞姩甯х紦鍐层€?*/
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

/* 鍦ㄥ鐢ㄩ┍鍔ㄥ抚缂撳啿涓粯鍒?24x24 鐨勨€滈儜鈥濆瓧鐐归樀銆?*/
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

/* 鍦ㄥ鐢ㄩ┍鍔ㄥ抚缂撳啿涓粯鍒舵矙婕忓浘鐗囦笅鏂圭殑鍛ㄦ湡鍜?TMP 骞冲潎娓╁害鏂囧瓧銆?*/
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

/* 浠庡綋鍓嶅浘鐗囪祫婧愭寜琛岃鍙栧苟璐村埌澶囩敤椹卞姩甯х紦鍐层€?*/
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

/* 鍦ㄥ鐢ㄩ┍鍔ㄤ富鐣岄潰鍙充晶鍔犺浇娌欐紡鍥剧墖鍏抽敭甯у苟缁樺埗骞冲潎娓╁害璇存槑銆?*/
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

/* 鍦ㄥ鐢?GIF 椤甸潰宸︿笂瑙掓爣璁板綋鍓嶅抚鏉ユ簮鎴栧け璐ョ姸鎬併€?*/
static void epd_alt_draw_gif_source_tag(const char *label)
{
    epd_alt_fill_rect(0, 0, 36, 10, 0);
    epd_alt_draw_string(2, 2, label, 1);
}

/* 澶囩敤椹卞姩涓嬬殑 GIF 璧勬簮缂哄け鎻愮ず銆?*/
static void epd_alt_draw_gif_missing(void)
{
    epd_alt_draw_gif_source_tag("NO SD");
    epd_alt_draw_string(48, 56, "NO SD", 2);
    epd_alt_draw_string(36, 80, "MASCOT.BIN", 1);
}

/* 鎸夎璇诲彇 GIF 璧勬簮甯у苟璐村埌澶囩敤椹卞姩甯х紦鍐层€?*/
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

/* 浣跨敤澶囩敤椹卞姩娓叉煋鍏ㄥ睆 GIF 鍔ㄧ敾褰撳墠甯с€?*/
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

/* 浣跨敤 5x7 瀛楀簱鍦ㄥ鐢ㄩ┍鍔ㄥ抚缂撳啿涓粯鍒朵竴涓瓧绗︺€?*/
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

/* 浣跨敤 5x7 瀛楀簱鍦ㄥ鐢ㄩ┍鍔ㄥ抚缂撳啿涓粯鍒跺瓧绗︿覆銆?*/
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

/* 在内置设置页字形表中查找指定 Unicode 码点。 */
static const uint8_t *epd_find_ui_glyph(uint16_t codepoint)
{
    uint8_t i;

    for (i = 0; i < (uint8_t)EPD_ARRAY_COUNT(epd_ui_glyphs); i++) {
        if (epd_ui_glyphs[i].codepoint == codepoint) {
            return epd_ui_glyphs[i].bitmap;
        }
    }
    return 0;
}

/* 使用 SSD1673 坐标系绘制一个内置中文 UI 字形。 */
static void epd_draw_ui_glyph(uint16_t x, uint16_t y, uint16_t codepoint)
{
    const uint8_t *bitmap;
    uint16_t py;

    bitmap = epd_find_ui_glyph(codepoint);
    if (bitmap == 0) {
        epd_draw_char(x, y, '?', 1);
        return;
    }
    for (py = 0; py < EPD_UI_GLYPH_H; py++) {
        epd_draw_1bpp_row(EPD_UI_GLYPH_W,
                          &bitmap[py * EPD_UI_GLYPH_STRIDE],
                          py,
                          (int16_t)x,
                          (int16_t)y,
                          1);
    }
}

/* 使用备用屏坐标系绘制一个内置中文 UI 字形。 */
static void epd_alt_draw_ui_glyph(uint16_t x, uint16_t y, uint16_t codepoint)
{
    const uint8_t *bitmap;
    uint16_t py;

    bitmap = epd_find_ui_glyph(codepoint);
    if (bitmap == 0) {
        epd_alt_draw_char(x, y, '?', 1);
        return;
    }
    for (py = 0; py < EPD_UI_GLYPH_H; py++) {
        epd_alt_draw_1bpp_row(EPD_UI_GLYPH_W,
                              &bitmap[py * EPD_UI_GLYPH_STRIDE],
                              py,
                              (int16_t)x,
                              (int16_t)y,
                              1);
    }
}

/* 绘制一段由 Unicode 码点数组表示的中文标签，并返回下一绘制位置。 */
static uint16_t epd_draw_ui_label(uint16_t x, uint16_t y, const uint16_t *label, uint8_t len)
{
    uint8_t i;
    uint16_t cursor;

    cursor = x;
    for (i = 0; i < len; i++) {
        epd_draw_ui_glyph(cursor, y, label[i]);
        cursor = (uint16_t)(cursor + EPD_UI_GLYPH_W);
    }
    return cursor;
}

/* 在备用屏上绘制中文标签，并返回下一绘制位置。 */
static uint16_t epd_alt_draw_ui_label(uint16_t x, uint16_t y, const uint16_t *label, uint8_t len)
{
    uint8_t i;
    uint16_t cursor;

    cursor = x;
    for (i = 0; i < len; i++) {
        epd_alt_draw_ui_glyph(cursor, y, label[i]);
        cursor = (uint16_t)(cursor + EPD_UI_GLYPH_W);
    }
    return cursor;
}

/* 计算 5x7 ASCII 字符串在指定缩放下占用的像素宽度。 */
static uint16_t epd_ascii_width(const char *s, uint8_t scale)
{
    uint16_t width;

    width = 0;
    while (*s) {
        width = (uint16_t)(width + 6u * scale);
        s++;
    }
    return width;
}

/* 按设置项生成右侧数值文本。 */
static void setting_to_value(uint8_t item, char *value)
{
    char *p;

    p = value;
    switch (item) {
    case 0:
        (void)append_u16(p, app_sample_interval());
        break;
    case 1:
        (void)append_t10(p, app_threshold_t10());
        break;
    case 2:
        (void)append_u16(p, app_storage_limit());
        break;
    case 3:
        (void)append_u16(p, app_alarm_duration_seconds());
        break;
    default:
        (void)append_u16(p, app_hourglass_seconds());
        break;
    }
}

/* 在 SSD1673 设置页中绘制一行中文设置项。 */
static void epd_draw_settings_row(uint8_t item, uint8_t selected, uint8_t editing, uint16_t y)
{
    char value[12];
    const EpdSettingRow *row;
    uint16_t unit_x;

    row = &epd_setting_rows[item];
    if (selected) {
        epd_draw_ui_glyph(0, y, editing ? 0x6539u : 0x9009u);
    }
    (void)epd_draw_ui_label(20, y, row->label, row->label_len);
    setting_to_value(item, value);
    epd_draw_string(112, (uint16_t)(y + 2u), value, 2);
    if (row->unit != 0) {
        unit_x = (uint16_t)(112u + epd_ascii_width(value, 2) + 4u);
        epd_draw_ui_glyph(unit_x, y, row->unit);
    }
}

/* 在备用屏设置页中绘制一行中文设置项。 */
static void epd_alt_draw_settings_row(uint8_t item, uint8_t selected, uint8_t editing, uint16_t y)
{
    char value[12];
    const EpdSettingRow *row;
    uint16_t unit_x;

    row = &epd_setting_rows[item];
    if (selected) {
        epd_alt_draw_ui_glyph(0, y, editing ? 0x6539u : 0x9009u);
    }
    (void)epd_alt_draw_ui_label(18, y, row->label, row->label_len);
    setting_to_value(item, value);
    epd_alt_draw_string(96, (uint16_t)(y + 5u), value, 1);
    if (row->unit != 0) {
        unit_x = (uint16_t)(96u + epd_ascii_width(value, 1) + 3u);
        epd_alt_draw_ui_glyph(unit_x, y, row->unit);
    }
}

/* 鍦?SSD1673 甯х紦鍐蹭腑缁樺埗涓€涓槄璇婚〉鐮佺偣銆?*/
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

/* 鍦ㄥ鐢ㄩ┍鍔ㄥ抚缂撳啿涓粯鍒朵竴涓槄璇婚〉鐮佺偣銆?*/
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

/* 浣跨敤 SSD1673 娓叉煋鏂囨湰闃呰椤点€?*/
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

/* 浣跨敤澶囩敤椹卞姩娓叉煋鏂囨湰闃呰椤点€?*/
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

/* 灏嗘俯搴︽牱鏈浆鎹负涓荤晫闈㈡樉绀虹敤鐨勫洓琛屾枃鏈€?*/
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

/* 灏嗕竴涓缃」杞崲涓鸿缃〉闈㈡樉绀烘枃鏈€?*/
/* 浣跨敤澶囩敤椹卞姩娓叉煋璁剧疆椤甸潰骞跺埛鏂板埌灞忓箷銆?*/
static void epd_alt_show_settings(void)
{
    uint8_t i;

    epd_alt_clear_buffer();
    (void)epd_alt_draw_ui_label(0, 0, epd_label_settings, (uint8_t)EPD_ARRAY_COUNT(epd_label_settings));
    for (i = 0; i < EPD_SETTINGS_ROWS; i++) {
        epd_alt_draw_settings_row(i,
                                  (uint8_t)(i == g_epd_settings_selected),
                                  g_epd_settings_editing,
                                  (uint16_t)(24u + i * 22u));
    }
    (void)epd_alt_write_buffer_to_screen(epd_buf);
}

/* 浣跨敤 SSD1673 娓叉煋璁剧疆椤甸潰骞跺眬閮ㄥ埛鏂板埌灞忓箷銆?*/
static void epd_show_settings(void)
{
    uint8_t i;

    epd_clear_buffer();
    (void)epd_draw_ui_label(0, 0, epd_label_settings, (uint8_t)EPD_ARRAY_COUNT(epd_label_settings));
    for (i = 0; i < EPD_SETTINGS_ROWS; i++) {
        epd_draw_settings_row(i,
                              (uint8_t)(i == g_epd_settings_selected),
                              g_epd_settings_editing,
                              (uint16_t)(22u + i * 20u));
    }
    epd_flush_partial();
}

/* 浣跨敤澶囩敤椹卞姩娓叉煋褰撳墠娓╁害涓荤晫闈€?*/
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

/* 浣跨敤 SSD1673 娓叉煋褰撳墠娓╁害涓荤晫闈€?*/
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

/* 浣跨敤 SSD1673 娓叉煋鍏ㄥ睆 GIF 鍔ㄧ敾褰撳墠甯с€?*/
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

/* 灏?Flash 鍘嗗彶璁板綍杞崲鎴愪竴琛岀畝鐭枃鏈€?*/
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

/* 鏍规嵁鍘嗗彶椤佃捣鐐瑰拰琛屽彿璁＄畻瀹為檯璁板綍涓嬫爣锛屾敮鎸佸惊鐜粴鍔ㄣ€?*/
static uint16_t history_row_index(uint16_t start, uint8_t row, uint16_t count)
{
    uint16_t index;

    index = (uint16_t)(start + row);
    if (index >= count && count > HISTORY_PAGE_ROWS) {
        index = (uint16_t)(index - count);
    }
    return index;
}

/* 鐢熸垚鍘嗗彶椤甸潰椤堕儴鐨勮褰曚綅缃拰鑷姩鎾斁鐘舵€佹枃鏈€?*/
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

/* 浣跨敤澶囩敤椹卞姩娓叉煋鍘嗗彶璁板綍椤甸潰銆?*/
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
    if (count <= HISTORY_PAGE_ROWS) {
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

    for (row = 0; row < HISTORY_PAGE_ROWS; row++) {
        index = history_row_index(start, row, count);
        if (index < count && history_get(index, &r)) {
            record_to_line(&r, line);
            epd_alt_draw_string(0, (uint16_t)(40u + row * 18u), line, 1);
        }
    }

    (void)epd_alt_write_buffer_to_screen(epd_buf);
}

/* 浣跨敤 SSD1673 娓叉煋鍘嗗彶璁板綍椤甸潰銆?*/
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
    if (count <= HISTORY_PAGE_ROWS) {
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

    for (row = 0; row < HISTORY_PAGE_ROWS; row++) {
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
