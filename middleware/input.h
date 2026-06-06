/*
 * input.h
 * 可移植输入事件接口：应用层只消费语义输入事件，不关心事件来自机械按键、
 * 电容触摸、GPIO 中断还是其他平台驱动。
 */
#ifndef INPUT_H
#define INPUT_H                                          /* 防止 input.h 被重复包含。 */

#include <stdint.h>

typedef enum {
    INPUT_EVENT_PRIMARY = 0u,                            /* 主动作输入，由平台层映射到具体物理输入。 */
    INPUT_EVENT_SECONDARY,                               /* 次动作输入，由平台层映射到具体物理输入。 */
    INPUT_EVENT_UP,                                      /* 向上输入，由平台层映射到具体物理输入。 */
    INPUT_EVENT_DOWN,                                    /* 向下输入，由平台层映射到具体物理输入。 */
    INPUT_EVENT_BACK,                                    /* 返回输入，由平台层映射到具体物理输入。 */
    INPUT_EVENT_CONFIRM,                                 /* 确认输入，由平台层映射到具体物理输入。 */
    INPUT_EVENT_COUNT                                    /* 输入事件数量。 */
} InputEvent;

#define INPUT_EVENT_BIT(event) ((uint8_t)(1u << (uint8_t)(event))) /* 将输入事件编号转换成事件位。 */

typedef void (*InputIsrWakeHook)(void);                  /* ISR 唤醒钩子，回调内部必须使用 ISR-safe API。 */

void input_init(void);                                   /* 初始化当前平台输入驱动。 */
void input_set_wake_hook(InputIsrWakeHook hook);         /* 设置输入中断唤醒钩子；传入 0 禁用。 */
uint8_t input_take_events(void);                         /* 取走已确认的新输入事件集合。 */
uint8_t input_pending(void);                             /* 判断是否存在未处理的新输入事件。 */

#endif
