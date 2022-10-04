#ifndef LAZYLOG_H
#define LAZYLOG_H

#include <stdbool.h>

enum zlog_lvl
{
    ZLOG_NOTSET,
    ZLOG_DEBUG,
    ZLOG_INFO,
    ZLOG_WARN,
    ZLOG_ERROR,
    ZLOG_FATAL,
};

#define zlog_debug(...)                                                        \
    zlog_print(ZLOG_DEBUG, __func__, __FILE__, __LINE__, __VA_ARGS__)
#define zlog_info(...)                                                         \
    zlog_print(ZLOG_INFO, __func__, __FILE__, __LINE__, __VA_ARGS__)
#define zlog_warn(...)                                                         \
    zlog_print(ZLOG_WARN, __func__, __FILE__, __LINE__, __VA_ARGS__)
#define zlog_error(...)                                                        \
    zlog_print(ZLOG_ERROR, __func__, __FILE__, __LINE__, __VA_ARGS__)
#define zlog_fatal(...)                                                        \
    zlog_print(ZLOG_FATAL, __func__, __FILE__, __LINE__, __VA_ARGS__)

typedef void zlog_print_fn_t(char const *string, void *);

void zlog_setup(zlog_print_fn_t *, void *arg, enum zlog_lvl);
void zlog_print(enum zlog_lvl, char const *func, char const *file, int line,
                char const *fmt, ...) __attribute__((format(printf, 5, 6)));

#endif
