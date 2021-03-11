#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define BUG(cond)                                                                                                      \
    do {                                                                                                               \
        if (!(cond))                                                                                                   \
            break;                                                                                                     \
        fprintf(stderr, "BUG: %s: %s: %d: %s\n", __FILE__, __func__, __LINE__, #cond);                                 \
        fflush(stderr);                                                                                                \
        exit(1);                                                                                                       \
    } while (0)

int fcopy_content(FILE* dst, FILE* src);
int fread_string(FILE* stream, char* str, size_t max_size);

#if defined(HAVE_ATTR_FORMAT)
#define ATTR_FORMAT __attribute__((format(printf, 1, 2)))
#else
#define ATTR_FORMAT
#endif

void die(char const* err, ...) ATTR_FORMAT;
void error(char const* err, ...) ATTR_FORMAT;

#endif
