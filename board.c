#include "board.h"
#include "app_state.h"

static volatile uint8_t g_sample_due = 1;
static volatile uint8_t g_second_count = 0;
static volatile uint8_t g_buzzer_alarm_seconds = 0;
static uint8_t g_buzzer_on = 0;

void delay_ms(uint16_t ms)
{
    while (ms--) {
        __delay_cycles(SMCLK_HZ / 1000u);
    }
}

void clock_init(void)
{
    UCSCTL3 = SELREF_2;
    UCSCTL4 = SELA_2 | SELS_4 | SELM_4;
    __bis_SR_register(SCG0);
    UCSCTL0 = 0;
    UCSCTL1 = DCORSEL_2;
    UCSCTL2 = FLLD_1 | 31;
    __bic_SR_register(SCG0);
    delay_ms(250);
}

void gpio_init(void)
{
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;

    P8DIR |= BIT1;
    P8OUT &= ~BIT1;

    BUZZER_SEL &= ~BUZZER_BIT;
    BUZZER_DIR |= BUZZER_BIT;
    BUZZER_REN |= BUZZER_BIT;
    BUZZER_OUT |= BUZZER_BIT;

    NTC_PORT_SEL |= NTC_PORT_BIT;
}

void board_toggle_heartbeat(void)
{
    P1OUT ^= BIT0;
}

void buzzer_set(uint8_t on)
{
    if (on && !g_buzzer_on) {
        BUZZER_OUT |= BUZZER_BIT;
        TA2CCR0 = (uint16_t)(BUZZER_HALF_PERIOD_TICKS - 1u);
        TA2CCTL0 = CCIE;
        TA2CTL = TASSEL_2 | MC_1 | TACLR;
        g_buzzer_on = 1;
    } else if (!on && g_buzzer_on) {
        TA2CTL = MC_0 | TACLR;
        TA2CCTL0 = 0;
        BUZZER_OUT |= BUZZER_BIT;
        g_buzzer_on = 0;
        g_buzzer_alarm_seconds = 0;
    }
}

void buzzer_beep(uint16_t ms)
{
    uint16_t toggles;
    uint8_t was_on;

    was_on = g_buzzer_on;
    if (was_on) {
        TA2CTL = MC_0 | TACLR;
        TA2CCTL0 = 0;
        g_buzzer_on = 0;
    }

    toggles = (uint16_t)(((uint32_t)ms * (SMCLK_HZ / 1000u)) / BUZZER_HALF_PERIOD_TICKS);
    if (toggles == 0) {
        toggles = 1;
    }

    while (toggles--) {
        BUZZER_OUT ^= BUZZER_BIT;
        __delay_cycles(BUZZER_HALF_PERIOD_TICKS);
    }

    BUZZER_OUT |= BUZZER_BIT;
    if (was_on) {
        buzzer_set(1);
    }
}

void buzzer_alert_for(uint8_t seconds)
{
    if (seconds == 0) {
        buzzer_set(0);
        return;
    }

    g_buzzer_alarm_seconds = seconds;
    buzzer_set(1);
}

void sample_timer_init(void)
{
    TA0CCR0 = 32768u - 1u;
    TA0CCTL0 = CCIE;
    TA0CTL = TASSEL_1 | MC_1 | TACLR;
}

uint8_t sample_timer_take_due(void)
{
    uint8_t due;

    due = g_sample_due;
    g_sample_due = 0;
    return due;
}

void sample_timer_force_due(void)
{
    g_sample_due = 1;
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
    if (g_buzzer_alarm_seconds > 0) {
        g_buzzer_alarm_seconds--;
        if (g_buzzer_alarm_seconds == 0) {
            buzzer_set(0);
        }
    }

    g_second_count++;
    if (g_second_count >= app_sample_interval()) {
        g_second_count = 0;
        g_sample_due = 1;
        __bic_SR_register_on_exit(LPM0_bits);
    }
}

#pragma vector=TIMER2_A0_VECTOR
__interrupt void TIMER2_A0_ISR(void)
{
    BUZZER_OUT ^= BUZZER_BIT;
}
