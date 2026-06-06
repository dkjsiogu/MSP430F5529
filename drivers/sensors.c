#include "sensors.h"
#include "board.h"

#define TMP421_REG_LOCAL_MSB        0x00 /* TMP421 本地温度整数部分寄存器。 */
#define TMP421_REG_CONFIG1_WRITE    0x09 /* TMP421 配置寄存器写地址。 */
#define TMP421_REG_CONV_RATE_WRITE  0x0B /* TMP421 转换速率寄存器写地址。 */
#define TMP421_REG_LOCAL_LSB        0x10 /* TMP421 本地温度小数部分寄存器。 */
#define TMP421_REG_CHANNEL_WRITE    0x21 /* TMP421 通道控制寄存器写地址。 */
#define TMP421_REG_MANUFACTURER_ID  0xFE /* TMP421 厂商 ID 寄存器。 */
#define TMP421_REG_DEVICE_ID        0xFF /* TMP421 器件 ID 寄存器。 */
#define TMP421_CONFIG_EXTENDED      0x04 /* TMP421 扩展温度范围配置位。 */
#define TMP421_CHANNEL_LOCAL_ONLY   0x01 /* TMP421 只启用本地温度通道的配置值。 */
#define TMP421_MANUFACTURER_ID      0x55 /* TMP421 期望的 TI 厂商 ID。 */
#define TMP421_DEVICE_ID            0x21 /* TMP421 典型器件 ID，用于识别芯片系列。 */

#define CAL_ADC_15T30               (*(const uint16_t *)0x1A1A) /* MSP430 1.5V 参考下 30 摄氏度校准 ADC 值。 */
#define CAL_ADC_15T85               (*(const uint16_t *)0x1A1C) /* MSP430 1.5V 参考下 85 摄氏度校准 ADC 值。 */
static const NtcPoint ntc_table[] = {
    {105385UL,  -200},
    { 77898UL,  -150},
    { 58246UL,  -100},
    { 44026UL,   -50},
    { 33621UL,     0},
    { 25925UL,    50},
    { 20175UL,   100},
    { 15837UL,   150},
    { 12535UL,   200},
    { 10000UL,   250},
    {  8037UL,   300},
    {  6506UL,   350},
    {  5301UL,   400},
    {  4348UL,   450},
    {  3588UL,   500},
    {  2978UL,   550},
    {  2486UL,   600},
    {  2086UL,   650},
    {  1760UL,   700},
    {  1492UL,   750},
    {  1270UL,   800},
    {  1087UL,   850},
    {   934UL,   900},
    {   805UL,   950},
    {   698UL,  1000}
};

static uint8_t g_tmp421_addr = 0;

/* 开关 ADC 内部 1.5V 参考源，片内温度采集时使用。 */
static void adc_power_reference(uint8_t on)
{
    if (on) {
        REFCTL0 = REFMSTR | REFVSEL_0 | REFON;
        delay_ms(2);
    } else {
        REFCTL0 = REFMSTR;
    }
}

/* 对指定 ADC12 通道执行一次单次转换并返回原始采样值。 */
static uint16_t adc_read_once(uint16_t inch, uint16_t sref, uint8_t use_internal_ref)
{
    uint16_t value;
    uint16_t timeout;

    if (use_internal_ref) {
        adc_power_reference(1);
    }

    ADC12CTL0 &= ~ADC12ENC;
    ADC12CTL0 = ADC12SHT0_15 | ADC12ON;
    ADC12CTL1 = ADC12SHP;
    ADC12MCTL0 = sref | inch;
    ADC12CTL0 |= ADC12ENC | ADC12SC;

    timeout = 60000u;
    while ((ADC12CTL1 & ADC12BUSY) && timeout > 0) {
        timeout--;
    }

    value = ADC12MEM0;
    ADC12CTL0 &= ~ADC12ENC;
    ADC12CTL0 &= ~ADC12ON;

    if (use_internal_ref) {
        adc_power_reference(0);
    }

    return value;
}

/* 对指定 ADC12 通道做 4 次采样求平均，降低瞬时噪声。 */
static uint16_t adc_read_avg(uint16_t inch, uint16_t sref, uint8_t use_internal_ref)
{
    uint8_t i;
    uint32_t sum;

    sum = 0;
    for (i = 0; i < 4; i++) {
        sum += adc_read_once(inch, sref, use_internal_ref);
    }
    return (uint16_t)(sum / 4u);
}

/* 将片内温度传感器 ADC 值换算为 0.1 摄氏度。 */
static int16_t die_adc_to_t10(uint16_t adc)
{
    uint16_t cal30;
    uint16_t cal85;
    long t10;
    long mv;

    cal30 = CAL_ADC_15T30;
    cal85 = CAL_ADC_15T85;

    if (cal30 != 0xFFFFu && cal85 != 0xFFFFu && cal85 > cal30) {
        t10 = ((long)adc - (long)cal30) * 550L;
        t10 = t10 / ((long)cal85 - (long)cal30) + 300L;
        return (int16_t)t10;
    }

    mv = ((long)adc * 1500L) / 4095L;
    t10 = ((mv - 986L) * 1000L) / 355L;
    return (int16_t)t10;
}

/* 根据 NTC 分压方向把 ADC 值统一成 NTC 两端电压占比。 */
static uint16_t ntc_scaled_adc(uint16_t adc)
{
#if NTC_PULLUP_TO_VCC
    return adc;
#else
    return (uint16_t)(4095u - adc);
#endif
}

/* 按分压公式把 ADC 值换算为 NTC 当前阻值，单位欧姆。 */
static uint32_t ntc_adc_to_ohms(uint16_t adc)
{
    uint16_t scaled_adc;

    scaled_adc = ntc_scaled_adc(adc);
    if (scaled_adc == 0u || scaled_adc >= 4095u) {
        return 0UL;
    }
    return ((uint32_t)NTC_SERIES_RESISTOR_OHMS * (uint32_t)scaled_adc) /
           (uint32_t)(4095u - scaled_adc);
}

/* 使用 NTC 阻值查表和线性插值把 ADC 值换算为 0.1 摄氏度。 */
static int16_t ntc_adc_to_t10(uint16_t adc)
{
    uint8_t i;
    uint32_t r;
    uint32_t r0;
    uint32_t r1;
    int16_t t0;
    int16_t t1;
    long t;
    const uint8_t n = (uint8_t)(sizeof(ntc_table) / sizeof(ntc_table[0]));

    r = ntc_adc_to_ohms(adc);
    if (r >= ntc_table[0].ohms) {
        return ntc_table[0].t10;
    }
    if (r <= ntc_table[n - 1u].ohms) {
        return ntc_table[n - 1u].t10;
    }

    for (i = 0; i < (uint8_t)(n - 1u); i++) {
        r0 = ntc_table[i].ohms;
        r1 = ntc_table[i + 1u].ohms;
        if (r <= r0 && r >= r1) {
            t0 = ntc_table[i].t10;
            t1 = ntc_table[i + 1u].t10;
            t = (long)t0 + ((long)r - (long)r0) * ((long)t1 - (long)t0) /
                ((long)r1 - (long)r0);
            return (int16_t)t;
        }
    }

    return INVALID_T10;
}

/* 判断 NTC ADC 值是否落在查表可计算的有效范围内。 */
static uint8_t ntc_adc_in_table_range(uint16_t adc)
{
    uint32_t r;
    const uint8_t n = (uint8_t)(sizeof(ntc_table) / sizeof(ntc_table[0]));

    r = ntc_adc_to_ohms(adc);
    if (r > ntc_table[0].ohms) {
        return 0;
    }
    if (r < ntc_table[n - 1u].ohms) {
        return 0;
    }
    return 1;
}

/* 通过主动拉高或拉低 NTC 引脚后再采样，用来判断接口是否悬空。 */
static uint16_t ntc_adc_after_drive(uint8_t drive_high)
{
    NTC_PORT_SEL &= ~NTC_PORT_BIT;
    NTC_PORT_REN &= ~NTC_PORT_BIT;
    NTC_PORT_DIR |= NTC_PORT_BIT;
    if (drive_high) {
        NTC_PORT_OUT |= NTC_PORT_BIT;
    } else {
        NTC_PORT_OUT &= ~NTC_PORT_BIT;
    }
    delay_ms(1);

    NTC_PORT_DIR &= ~NTC_PORT_BIT;
    NTC_PORT_SEL |= NTC_PORT_BIT;
    return adc_read_avg(NTC_ADC_INCH, ADC12SREF_0, 0);
}

/* 判断 NTC 输入端是否真的接入热敏电阻，避免未接线时显示假温度。 */
static uint8_t ntc_input_present(void)
{
    uint16_t drive_high_adc;
    uint16_t drive_low_adc;
    uint16_t diff;

    drive_high_adc = ntc_adc_after_drive(1);
    drive_low_adc = ntc_adc_after_drive(0);

    NTC_PORT_DIR &= ~NTC_PORT_BIT;
    NTC_PORT_REN &= ~NTC_PORT_BIT;
    NTC_PORT_SEL |= NTC_PORT_BIT;

    if (drive_high_adc > drive_low_adc) {
        diff = (uint16_t)(drive_high_adc - drive_low_adc);
    } else {
        diff = (uint16_t)(drive_low_adc - drive_high_adc);
    }

    if (drive_high_adc > 3500u && drive_low_adc < 600u) {
        return 0;
    }
    if (diff > 1200u) {
        return 0;
    }
    return 1;
}

/* 初始化 UCB0 I2C 主机，用于访问 TMP421。 */
static void i2c_init(void)
{
    P3SEL |= BIT0 | BIT1;
    UCB0CTL1 |= UCSWRST;
    UCB0CTL0 = UCMST | UCMODE_3 | UCSYNC;
    UCB0CTL1 = UCSWRST | UCSSEL_2;
    UCB0BR0 = 160;
    UCB0BR1 = 0;
    UCB0CTL1 &= ~UCSWRST;
}

/* 等待 I2C 总线空闲，超时返回 0。 */
static uint8_t i2c_wait_bus_free(void)
{
    uint16_t timeout;

    timeout = 60000u;
    while ((UCB0STAT & UCBBUSY) && timeout > 0) {
        timeout--;
    }
    return (uint8_t)(timeout > 0);
}

/* 检查 I2C NACK，并在收到 NACK 时发送 STOP。 */
static uint8_t i2c_check_nack(void)
{
    if (UCB0IFG & UCNACKIFG) {
        UCB0IFG &= ~UCNACKIFG;
        UCB0CTL1 |= UCTXSTP;
        return 1;
    }
    return 0;
}

/* 等待 I2C 发送缓冲区可写，同时处理 NACK。 */
static uint8_t i2c_wait_txifg(void)
{
    uint16_t timeout;

    timeout = 60000u;
    while (!(UCB0IFG & UCTXIFG) && timeout > 0) {
        if (i2c_check_nack()) {
            return 0;
        }
        timeout--;
    }
    return (uint8_t)(timeout > 0);
}

/* 等待 I2C 接收缓冲区有数据，同时处理 NACK。 */
static uint8_t i2c_wait_rxifg(void)
{
    uint16_t timeout;

    timeout = 60000u;
    while (!(UCB0IFG & UCRXIFG) && timeout > 0) {
        if (i2c_check_nack()) {
            return 0;
        }
        timeout--;
    }
    return (uint8_t)(timeout > 0);
}

/* 等待 I2C STOP 条件发送完成。 */
static uint8_t i2c_wait_stop(void)
{
    uint16_t timeout;

    timeout = 60000u;
    while ((UCB0CTL1 & UCTXSTP) && timeout > 0) {
        timeout--;
    }
    return (uint8_t)(timeout > 0);
}

/* 探测指定 I2C 地址是否有设备应答。 */
static uint8_t i2c_probe(uint8_t addr)
{
    uint16_t timeout;
    uint8_t ack;

    if (!i2c_wait_bus_free()) {
        return 0;
    }

    UCB0I2CSA = addr;
    UCB0CTL1 |= UCTR | UCTXSTT;

    timeout = 60000u;
    ack = 0;
    while (timeout > 0) {
        if (UCB0IFG & UCNACKIFG) {
            UCB0IFG &= ~UCNACKIFG;
            ack = 0;
            break;
        }
        if (UCB0IFG & UCTXIFG) {
            ack = 1;
            break;
        }
        timeout--;
    }

    UCB0CTL1 |= UCTXSTP;
    (void)i2c_wait_stop();
    return ack;
}

/* 向指定 I2C 设备的寄存器写入一个字节。 */
static uint8_t i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value)
{
    if (!i2c_wait_bus_free()) {
        return 0;
    }

    UCB0I2CSA = addr;
    UCB0CTL1 |= UCTR | UCTXSTT;
    if (!i2c_wait_txifg()) {
        return 0;
    }
    UCB0TXBUF = reg;
    if (!i2c_wait_txifg()) {
        return 0;
    }
    UCB0TXBUF = value;
    if (!i2c_wait_txifg()) {
        return 0;
    }
    UCB0CTL1 |= UCTXSTP;
    return i2c_wait_stop();
}

/* 从指定 I2C 设备的寄存器读取一个字节。 */
static uint8_t i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *value)
{
    uint16_t timeout;

    if (!i2c_wait_bus_free()) {
        return 0;
    }

    UCB0I2CSA = addr;
    UCB0CTL1 |= UCTR | UCTXSTT;
    if (!i2c_wait_txifg()) {
        return 0;
    }
    UCB0TXBUF = reg;
    if (!i2c_wait_txifg()) {
        return 0;
    }

    UCB0CTL1 &= ~UCTR;
    UCB0CTL1 |= UCTXSTT;

    timeout = 60000u;
    while ((UCB0CTL1 & UCTXSTT) && timeout > 0) {
        if (i2c_check_nack()) {
            return 0;
        }
        timeout--;
    }
    if (timeout == 0) {
        UCB0CTL1 |= UCTXSTP;
        return 0;
    }

    UCB0CTL1 |= UCTXSTP;
    if (!i2c_wait_rxifg()) {
        return 0;
    }
    *value = UCB0RXBUF;
    return i2c_wait_stop();
}

/* 判断读到的器件 ID 是否属于 TMP421/TMP42x 兼容系列。 */
static uint8_t tmp421_is_device_id(uint8_t id)
{
    return (uint8_t)(id == 0x21u || id == 0x22u || id == 0x23u ||
                     id == 0x41u || id == 0x42u);
}

uint8_t tmp421_detect(void)
{
    const uint8_t candidates[] = {0x2Au, 0x4Cu, 0x4Du, 0x4Eu, 0x4Fu};
    uint8_t i;
    uint8_t manf;
    uint8_t dev;
    uint8_t local;

    for (i = 0; i < (uint8_t)sizeof(candidates); i++) {
        if (!i2c_probe(candidates[i])) {
            continue;
        }
        if (i2c_read_reg(candidates[i], TMP421_REG_MANUFACTURER_ID, &manf) &&
            i2c_read_reg(candidates[i], TMP421_REG_DEVICE_ID, &dev) &&
            manf == TMP421_MANUFACTURER_ID && tmp421_is_device_id(dev)) {
            g_tmp421_addr = candidates[i];
            return 1;
        }
        if (candidates[i] == 0x2Au &&
            i2c_read_reg(candidates[i], TMP421_REG_LOCAL_MSB, &local)) {
            g_tmp421_addr = candidates[i];
            return 1;
        }
        if (candidates[i] != 0x2Au) {
            continue;
        }
    }

    g_tmp421_addr = 0;
    return 0;
}

/* 检测并配置 TMP421，让其输出本地温度并使用扩展温度格式。 */
static void tmp421_init(void)
{
    if (!tmp421_detect()) {
        return;
    }

    (void)i2c_write_reg(g_tmp421_addr, TMP421_REG_CONFIG1_WRITE, TMP421_CONFIG_EXTENDED);
    (void)i2c_write_reg(g_tmp421_addr, TMP421_REG_CHANNEL_WRITE, TMP421_CHANNEL_LOCAL_ONLY);
    (void)i2c_write_reg(g_tmp421_addr, TMP421_REG_CONV_RATE_WRITE, 0x07);
}

/* 读取 TMP421 指定温度寄存器并换算成 0.1 摄氏度。 */
static uint8_t tmp421_read_t10(uint8_t msb_reg, uint8_t lsb_reg, int16_t *t10)
{
    uint8_t msb;
    uint8_t lsb;
    uint8_t frac;
    long value;

    if (!g_tmp421_addr) {
        *t10 = INVALID_T10;
        return 0;
    }

    if (!i2c_read_reg(g_tmp421_addr, msb_reg, &msb)) {
        *t10 = INVALID_T10;
        return 0;
    }
    if (!i2c_read_reg(g_tmp421_addr, lsb_reg, &lsb)) {
        lsb = 0;
    }
    frac = (uint8_t)(lsb >> 4);
    value = ((long)msb * 160L + (long)frac * 10L + 8L) / 16L;
    value -= 640L;
    *t10 = (int16_t)value;
    return 1;
}

uint8_t tmp421_addr(void)
{
    return g_tmp421_addr;
}

void sensors_init(void)
{
    i2c_init();
    tmp421_init();
}
void collect_sample(TempSample *s)
{
    uint16_t adc;
    uint8_t tmp_ok;

    s->flags = 0;
    s->reserved_t10 = INVALID_T10;

    adc = adc_read_avg(ADC12INCH_10, ADC12SREF_1, 1);
    s->die_t10 = die_adc_to_t10(adc);
    s->flags |= FLAG_DIE_OK;

    adc = adc_read_avg(NTC_ADC_INCH, ADC12SREF_0, 0);
    if (ntc_input_present() && ntc_adc_in_table_range(adc)) {
        s->ntc_t10 = ntc_adc_to_t10(adc);
        s->flags |= FLAG_NTC_OK;
    } else {
        s->ntc_t10 = INVALID_T10;
    }

    tmp_ok = tmp421_read_t10(TMP421_REG_LOCAL_MSB, TMP421_REG_LOCAL_LSB, &s->tmp_local_t10);
    if (tmp_ok) {
        s->flags |= FLAG_TMP_LOCAL_OK;
    }
}

