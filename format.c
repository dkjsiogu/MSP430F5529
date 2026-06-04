#include "format.h"

char *append_str(char *p, const char *s)
{
    while (*s) {
        *p++ = *s++;
    }
    *p = 0;
    return p;
}

char *append_u16(char *p, uint16_t v)
{
    char buf[6];
    uint8_t i;

    if (v == 0) {
        *p++ = '0';
        *p = 0;
        return p;
    }

    i = 0;
    while (v > 0 && i < sizeof(buf)) {
        buf[i++] = (char)('0' + (v % 10));
        v = (uint16_t)(v / 10);
    }
    while (i > 0) {
        *p++ = buf[--i];
    }
    *p = 0;
    return p;
}

char *append_t10(char *p, int16_t t10)
{
    uint16_t a;

    if (t10 == INVALID_T10) {
        return append_str(p, "--.-");
    }

    if (t10 < 0) {
        *p++ = '-';
        a = (uint16_t)(-t10);
    } else {
        a = (uint16_t)t10;
    }

    p = append_u16(p, (uint16_t)(a / 10));
    *p++ = '.';
    *p++ = (char)('0' + (a % 10));
    *p = 0;
    return p;
}

