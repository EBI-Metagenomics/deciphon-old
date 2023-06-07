#ifndef ECHO_H
#define ECHO_H

#include <stdio.h>

void echof(FILE *restrict, char const *fmt, ...)
    __attribute__((format(printf, 2, 3)));
void echo(char const *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif
