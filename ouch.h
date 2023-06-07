#ifndef OUCH_H
#define OUCH_H

#include <stdio.h>

void ouch(char const *restrict fmt, ...) __attribute__((format(printf, 1, 2)));

#endif
