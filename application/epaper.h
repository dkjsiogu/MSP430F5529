/*
 * epaper.h
 * 墨水屏渲染接口：管理当前温度页、历史页、统一渲染任务，
 * 以及本地控制使用的屏幕驱动选择。
 */
#ifndef EPAPER_H
#define EPAPER_H                                        /* 防止 epaper.h 被重复包含。 */

#include "app_types.h"

void epd_init(void);                                       /* 初始化 EPD GPIO、SPI、控制器状态和首次渲染。 */
void epd_show_current_auto(const TempSample *s);           /* 提交最新温度样本作为当前目标画面。 */
void epd_show_history_page(uint16_t start);                /* 提交从指定记录序号开始的历史页画面。 */
void epd_show_history_playback(void);                      /* 进入 Flash 历史记录自动滚动播放页。 */
void epd_show_gif_playback(void);                          /* 进入全屏 GIF 动画播放页，并按固定帧率循环刷新。 */
void epd_gif_prev_asset(void);                             /* GIF 页面切换到上一个可用 SD 动图资源。 */
void epd_gif_next_asset(void);                             /* GIF 页面切换到下一个可用 SD 动图资源。 */
void epd_show_text_reader(void);                           /* 进入 SD 卡 BOOK.TXT 阅读页。 */
void epd_text_prev_page(void);                             /* 阅读页切换到上一页文本。 */
void epd_text_next_page(void);                             /* 阅读页切换到下一页文本。 */
void epd_show_settings_page(uint8_t selected, uint8_t editing); /* 提交设置页画面，selected 为当前项目，editing 表示正在改值。 */
void epd_render_task(void);                                /* 通过局部刷新渲染一个待处理的整屏目标画面。 */
uint8_t epd_render_pending(void);                          /* 返回当前是否还有待渲染画面。 */
void epd_force_next_current_refresh(void);                 /* 标记当前温度页需要在下次任务中重新渲染。 */
void epd_full_refresh_once(void);                          /* 执行一次启动同类全屏刷新，并重新渲染目标画面。 */
uint8_t epd_auto_enabled(void);                            /* 返回当前是否启用自动温度页刷新。 */
void epd_resume_auto(void);                                /* 恢复自动温度页刷新。 */
void epd_use_ssd(void);                                    /* 选择 SSD1673 墨水屏驱动并刷新当前页。 */
void epd_use_alt_auto(void);                               /* 选择备用墨水屏驱动并恢复自动刷新。 */

#endif
