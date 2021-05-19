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

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define warn(code, ...) __imm_log(IMM_WARN, code, __VA_ARGS__)
#define error(code, ...) __imm_log(IMM_ERROR, code, __VA_ARGS__)
#define fatal(code, ...) __imm_log(IMM_FATAL, code, __VA_ARGS__)

#define xmalloc(x) __malloc((x), __FILE__, __LINE__)
#define xmemcpy(d, s, c) __memcpy((d), (s), (c), __FILE__, __LINE__)
#define xrealloc(ptr, new_size) __realloc((ptr), (new_size), __FILE__, __LINE__)
#define xstrdup(x) __strdup((x), __FILE__, __LINE__)

static inline void *__memcpy(void *restrict dest, const void *restrict src,
                             size_t count, char const file[static 1], int line)
    __attribute__((nonnull(1, 2)));

static inline void *__realloc(void *ptr, size_t new_size,
                              char const file[static 1], int line);

static inline char *__strdup(char const *str, char const file[static 1],
                             int line) __attribute__((nonnull(1)));

static inline char *__strdup(char const *str, char const file[static 1],
                             int line) __attribute__((nonnull(1)));

static inline void *__growmem(void *restrict ptr, size_t count, size_t size,
                              size_t *capacity, char const file[static 1],
                              int line) __attribute__((nonnull(1, 4)));

static inline void free_if(void const *ptr)
{
    if (ptr)
        free((void *)ptr);
}

static inline void *__malloc(size_t size, char const file[static 1], int line)
{
    void *ptr = malloc(size);
    if (!ptr)
        fatal(IMM_IOERROR, "failed to malloc");
    return ptr;
}

static inline void *__memcpy(void *restrict dest, const void *restrict src,
                             size_t count, char const file[static 1], int line)
{
    void *ptr = memcpy(dest, src, count);
    if (!ptr)
        fatal(IMM_IOERROR, "failed to memcpy");
    return ptr;
}

static inline void *__realloc(void *ptr, size_t new_size,
                              char const file[static 1], int line)
{
    void *new_ptr = realloc(ptr, new_size);
    if (!new_ptr)
        fatal(IMM_IOERROR, "failed to realloc");
    return new_ptr;
}

static inline char *__strdup(char const *str, char const file[static 1],
                             int line)
{
    char *new = strdup(str);
    if (!new)
        fatal(IMM_IOERROR, "failed to strdup");
    return new;
}

#endif
