#include "strhash.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static long fletcher16(uint8_t *buf, size_t size);

long strhash(char *str) { return fletcher16((uint8_t *)str, strlen(str)); }

static long fletcher16(uint8_t *buf, size_t size)
{
    uint16_t sum1 = 0;
    uint16_t sum2 = 0;
    for (size_t i = 0; i < size; ++i)
    {
        sum1 = (sum1 + buf[i]) % 255;
        sum2 = (sum2 + sum1) % 255;
    }

    return (sum2 << 8) | sum1;
}
