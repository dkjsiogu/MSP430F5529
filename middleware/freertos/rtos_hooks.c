#include "FreeRTOS.h"
#include "task.h"

#include <msp430.h>

#pragma DATA_SECTION(g_idle_task_tcb, ".bss:usbram")
static StaticTask_t g_idle_task_tcb;
#pragma DATA_SECTION(g_idle_task_stack, ".bss:usbram")
static StackType_t g_idle_task_stack[configMINIMAL_STACK_SIZE];

void vApplicationSetupTimerInterrupt(void)
{
    TA1CCR0 = (uint16_t)((32768u / configTICK_RATE_HZ) - 1u);
    TA1CCTL0 = CCIE;
    TA1CTL = TASSEL_1 | MC_1 | TACLR;
}

void vApplicationGetIdleTaskMemory(StaticTask_t **ppx_idle_task_tcb,
                                   StackType_t **ppx_idle_task_stack,
                                   configSTACK_DEPTH_TYPE *pux_idle_task_stack_size)
{
    *ppx_idle_task_tcb = &g_idle_task_tcb;
    *ppx_idle_task_stack = g_idle_task_stack;
    *pux_idle_task_stack_size = configMINIMAL_STACK_SIZE;
}
