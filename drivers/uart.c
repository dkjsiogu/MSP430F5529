#include "uart.h"

#include "platform_config.h"

static volatile uint8_t g_uart_rx_char = 0;
static UartIsrWakeHook g_uart_rx_hook = 0;

void uart_init(void)
{
    P4SEL &= ~BIT4;
    P4DIR &= ~(BIT4 | BIT5);
    P4SEL |= BIT5;
    UCA1CTL1 |= UCSWRST;
    UCA1CTL1 |= UCSSEL_2;
    UCA1BR0 = 104;
    UCA1BR1 = 0;
    UCA1MCTL = UCBRF_3 | UCBRS_0 | UCOS16;
    UCA1CTL1 &= ~UCSWRST;
    UCA1IE |= UCRXIE;
}

void uart_set_rx_hook(UartIsrWakeHook hook)
{
    g_uart_rx_hook = hook;
}

uint8_t uart_take_rx(void)
{
    uint8_t c;

    c = g_uart_rx_char;
    g_uart_rx_char = 0;
    return c;
}
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
    uint8_t c;

    switch (__even_in_range(UCA1IV, 4)) {
    case 2:
        c = UCA1RXBUF;
        if (c != '\r' && c != '\n' && c != ' ' && c != '\t') {
            g_uart_rx_char = c;
            if (g_uart_rx_hook != 0) {
                g_uart_rx_hook();
            }
            __bic_SR_register_on_exit(LPM0_bits);
        }
        break;
    default:
        break;
    }
}
