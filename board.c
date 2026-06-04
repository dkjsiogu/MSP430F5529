#include "board.h"
#include "app_state.h"

static volatile uint8_t g_sample_due = 1;
static volatile uint8_t g_second_count = 0;
static volatile uint8_t g_subsecond_ticks = 0;
static volatile uint16_t g_tick_10ms = 0;
static volatile uint8_t g_buzzer_alarm_seconds = 0;
static uint8_t g_buzzer_on = 0;

/* 按 TI 推荐流程把 PMM 核心电压提高一级，保证高主频运行稳定。 */
static uint8_t pmm_set_vcore_up(uint8_t level)
{
    uint16_t pmmrie_backup;
    uint16_t svsmhctl_backup;
    uint16_t svsmlctl_backup;

    PMMCTL0_H = PMMPW_H;

    pmmrie_backup = PMMRIE;
    PMMRIE &= (uint16_t)~(SVMHVLRPE | SVSHPE | SVMLVLRPE | SVSLPE | SVMHVLRIE |
                          SVMHIE | SVSMHDLYIE | SVMLVLRIE | SVMLIE | SVSMLDLYIE);
    svsmhctl_backup = SVSMHCTL;
    svsmlctl_backup = SVSMLCTL;

    PMMIFG = 0;
    SVSMHCTL = SVMHE | SVSHE | (uint16_t)(SVSMHRRL0 * level);
    while ((PMMIFG & SVSMHDLYIFG) == 0) {
        ;
    }
    PMMIFG &= (uint16_t)~SVSMHDLYIFG;

    if (PMMIFG & SVMHIFG) {
        PMMIFG &= (uint16_t)~SVSMHDLYIFG;
        SVSMHCTL = svsmhctl_backup;
        while ((PMMIFG & SVSMHDLYIFG) == 0) {
            ;
        }
        PMMIFG &= (uint16_t)~(SVMHVLRIFG | SVMHIFG | SVSMHDLYIFG |
                              SVMLVLRIFG | SVMLIFG | SVSMLDLYIFG);
        PMMRIE = pmmrie_backup;
        PMMCTL0_H = 0;
        return 0;
    }

    SVSMHCTL |= (uint16_t)(SVSHRVL0 * level);
    while ((PMMIFG & SVSMHDLYIFG) == 0) {
        ;
    }
    PMMIFG &= (uint16_t)~SVSMHDLYIFG;

    PMMCTL0_L = (uint8_t)(PMMCOREV0 * level);
    SVSMLCTL = SVMLE | (uint16_t)(SVSMLRRL0 * level) |
               SVSLE | (uint16_t)(SVSLRVL0 * level);
    while ((PMMIFG & SVSMLDLYIFG) == 0) {
        ;
    }
    PMMIFG &= (uint16_t)~SVSMLDLYIFG;

    SVSMLCTL &= (uint16_t)(SVSLRVL0 | SVSLRVL1 | SVSMLRRL0 | SVSMLRRL1 | SVSMLRRL2);
    svsmlctl_backup &= (uint16_t)~(SVSLRVL0 | SVSLRVL1 | SVSMLRRL0 | SVSMLRRL1 | SVSMLRRL2);
    SVSMLCTL |= svsmlctl_backup;

    SVSMHCTL &= (uint16_t)(SVSHRVL0 | SVSHRVL1 | SVSMHRRL0 | SVSMHRRL1 | SVSMHRRL2);
    svsmhctl_backup &= (uint16_t)~(SVSHRVL0 | SVSHRVL1 | SVSMHRRL0 | SVSMHRRL1 | SVSMHRRL2);
    SVSMHCTL |= svsmhctl_backup;

    while (((PMMIFG & SVSMLDLYIFG) == 0) && ((PMMIFG & SVSMHDLYIFG) == 0)) {
        ;
    }
    PMMIFG &= (uint16_t)~(SVMHVLRIFG | SVMHIFG | SVSMHDLYIFG |
                          SVMLVLRIFG | SVMLIFG | SVSMLDLYIFG);

    PMMRIE = pmmrie_backup;
    PMMCTL0_H = 0;
    return 1;
}

/* 逐级提高 VCore 到目标等级，16MHz 使用 PMMCOREV_2。 */
static void pmm_set_vcore(uint8_t target_level)
{
    uint8_t level;
    uint8_t current_level;

    target_level &= PMMCOREV_3;
    current_level = (uint8_t)(PMMCTL0 & PMMCOREV_3);
    for (level = (uint8_t)(current_level + 1u); level <= target_level; level++) {
        while (!pmm_set_vcore_up(level)) {
            ;
        }
    }
}

void delay_ms(uint16_t ms)
{
    while (ms--) {
        __delay_cycles(MCLK_HZ / 1000u);
    }
}

void clock_init(void)
{
    pmm_set_vcore(PMMCOREV_2);

    UCSCTL3 = SELREF_2;
    UCSCTL4 = SELA_2 | SELS_4 | SELM_4;
    __bis_SR_register(SCG0);
    UCSCTL0 = 0;
    UCSCTL1 = DCORSEL_5;
    UCSCTL2 = FLLD_0 | 487u;
    UCSCTL5 = 0;
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
    TA0CCR0 = (uint16_t)((32768u / BOARD_TICK_HZ) - 1u);
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

uint16_t board_tick10(void)
{
    return g_tick_10ms;
}

uint8_t board_tick10_elapsed(uint16_t start_tick, uint16_t ticks)
{
    return (uint8_t)((uint16_t)(board_tick10() - start_tick) >= ticks);
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
    g_tick_10ms++;
    g_subsecond_ticks++;

    if (g_subsecond_ticks >= BOARD_TICKS_PER_SECOND) {
        g_subsecond_ticks = 0;

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
        }
    }

    __bic_SR_register_on_exit(LPM0_bits);
}

#pragma vector=TIMER2_A0_VECTOR
__interrupt void TIMER2_A0_ISR(void)
{
    BUZZER_OUT ^= BUZZER_BIT;
}
