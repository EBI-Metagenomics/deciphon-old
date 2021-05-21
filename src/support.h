#ifndef SUPPORT_H
#define SUPPORT_H

#include "imm/compiler.h"
#include "imm/log.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char const *tempfile(char const *filepath);

int fcopy(FILE *dst, FILE *src);

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define warn(code, ...) __imm_log(IMM_WARN, code, __VA_ARGS__)
#define error(code, ...) __imm_log(IMM_ERROR, code, __VA_ARGS__)
#define fatal(code, ...) __imm_log(IMM_FATAL, code, __VA_ARGS__)

#define xcalloc(n, s) __calloc((n), (s), __FILE__, __LINE__)
#define xmalloc(x) __malloc((x), __FILE__, __LINE__)
#define xmemcpy(d, s, c) __memcpy((d), (s), (c), __FILE__, __LINE__)
#define xmemset(d, ch, count) __memset((d), (ch), (count), __FILE__, __LINE__)
#define xrealloc(ptr, new_size) __realloc((ptr), (new_size), __FILE__, __LINE__)
#define xstrdup(x) __strdup((x), __FILE__, __LINE__)

#define xdel(x)                                                                \
    do                                                                         \
    {                                                                          \
        x = del(x);                                                            \
    } while (0);

static inline void *__memcpy(void *restrict dest, const void *restrict src,
                             size_t count, char const file[static 1], int line)
    __attribute__((nonnull(1, 2)));

static inline void *__memset(void *dest, int ch, size_t count,
                             char const file[static 1], int line)
    __attribute__((nonnull(1)));

static inline void *__realloc(void *ptr, size_t new_size,
                              char const file[static 1], int line);

static inline char *__strdup(char const *str, char const file[static 1],
                             int line) __attribute__((nonnull(1)));

static inline void *del(void const *ptr)
{
    if (ptr)
        free((void *)ptr);
    return NULL;
}

static inline void *__calloc(size_t num, size_t size, char const file[static 1],
                             int line)
{
    void *ptr = calloc(num, size);
    if (!ptr)
        __imm_log_impl(IMM_FATAL, IMM_IOERROR, file, line, "failed to calloc");
    return ptr;
}

static inline void *__malloc(size_t size, char const file[static 1], int line)
{
    void *ptr = malloc(size);
    if (!ptr)
        __imm_log_impl(IMM_FATAL, IMM_IOERROR, file, line, "failed to malloc");
    return ptr;
}

static inline void *__memcpy(void *restrict dest, const void *restrict src,
                             size_t count, char const file[static 1], int line)
{
    void *ptr = memcpy(dest, src, count);
    if (!ptr)
        __imm_log_impl(IMM_FATAL, IMM_IOERROR, file, line, "failed to memcpy");
    return ptr;
}

static inline void *__memset(void *dest, int ch, size_t count,
                             char const file[static 1], int line)
{
    void *ptr = memset(dest, ch, count);
    if (!ptr)
        __imm_log_impl(IMM_FATAL, IMM_IOERROR, file, line, "failed to memset");
    return ptr;
}

static inline void *__realloc(void *ptr, size_t new_size,
                              char const file[static 1], int line)
{
    void *new_ptr = realloc(ptr, new_size);
    if (!new_ptr)
        __imm_log_impl(IMM_FATAL, IMM_IOERROR, file, line, "failed to realloc");
    return new_ptr;
}

static inline char *__strdup(char const *str, char const file[static 1],
                             int line)
{
    char *new = strdup(str);
    if (!new)
        __imm_log_impl(IMM_FATAL, IMM_IOERROR, file, line, "failed to strdup");
    return new;
}

#endif
