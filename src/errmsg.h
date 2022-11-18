#ifndef ERRMSG_H
#define ERRMSG_H

#include "deciphon_limits.h"

#define ERRMSG_DECLARE(name) char name[ERROR_SIZE];

char const *errfmt(char *dst, char const *fmt, ...)
    __attribute__((format(printf, 2, 3)));

#endif
