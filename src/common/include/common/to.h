#ifndef TO_H
#define TO_H

#include "common/export.h"
#include <stdbool.h>
#include <stdint.h>

EXPORT bool to_double(char const *str, double *val);
EXPORT bool to_int64(char const *str, int64_t *val);
EXPORT bool to_int64l(unsigned len, char const *str, int64_t *val);
EXPORT bool to_int(char const *str, int *val);
EXPORT unsigned to_str(char *str, uint16_t num);

#endif
