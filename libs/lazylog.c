#include "lazylog.h"
#include <stdarg.h>
#include <stdatomic.h>
#include <string.h>
#include <time.h>

#define NANOPRINTF_IMPLEMENTATION 1
#define NANOPRINTF_VISIBILITY_STATIC 1
#include "nanoprintf.h"

#define array_size(arr) (sizeof(arr) / sizeof((arr)[0]))

#ifdef FILEPATH_SIZE
#undef FILEPATH_SIZE
#endif

#define FILEPATH_SIZE 2048

static enum zlog_lvl level = ZLOG_INFO;
static char buf[FILEPATH_SIZE * 2] = {0};
static zlog_print_fn_t *print_callb = NULL;
static void *print_arg = NULL;
static const char *strings[] = {"NOTSET", "DEBUG", "INFO",
                                "WARN",   "ERROR", "FATAL"};
static atomic_flag flag = ATOMIC_FLAG_INIT;
static char const *root = NULL;

static inline void lock(void)
{
    while (atomic_flag_test_and_set(&flag))
        ;
}

static inline void unlock(void) { atomic_flag_clear(&flag); }

void zlog_setup(enum zlog_lvl lvl, zlog_print_fn_t *print, void *arg)
{
    level = lvl;
    print_callb = print;
    print_arg = arg;
}

void zlog_setroot(char const *r) { root = r; }

static char const *short_file(char const *file, char const *root);

void zlog_print(enum zlog_lvl lvl, char const *func, char const *file, int line,
                char const *fmt, ...)
{
    lock();
    if (!print_callb) goto noop;

    time_t timer = time(NULL);
    struct tm *tm_info = localtime(&timer);

    char *p = buf;
    char *end = buf + sizeof(buf);

    p += strftime(buf, sizeof(buf), "%H:%M:%S", tm_info);

    if (root) file = short_file(file, root);

    if (lvl >= level)
    {
        p += npf_snprintf(p, end - p, " | %-5s", strings[lvl]);
        p += npf_snprintf(p, end - p, " | %s:%s:%d - ", func, file, line);

        va_list ap;
        va_start(ap, fmt);
        p += npf_vsnprintf(p, end - p, fmt, ap);
        va_end(ap);

        p += npf_snprintf(p, end - p, "%c", '\n');

        if (p >= end) p = end - 1;
    }

    if (p + 1 == end) *(p - 1) = '\n';
    *p = '\0';
    (*print_callb)(buf, print_arg);

noop:
    unlock();
}

static size_t prefix_size(char const *file, char const *root);

static char const *short_file(char const *file, char const *root)
{
    return file + prefix_size(file, root);
}

static char *dirname(char *path);
static char *basename(char const *path);

static size_t prefix_size(char const *file, char const *root)
{
    static char tmp[FILEPATH_SIZE] = {0};
    strcpy(tmp, file);

    char *dir = dirname(tmp);
    char const *name = basename(tmp);

    while (strcmp(dir, "/") && strcmp(dir, ".") && strcmp(name, root))
    {
        dir = dirname(tmp);
        name = basename(tmp);
    }

    return !strcmp(name, root) ? strlen(tmp) + 1 : 0;
}

static char *dirname(char *path)
{
    size_t i;
    if (!path || !*path) return ".";
    i = strlen(path) - 1;
    for (; path[i] == '/'; i--)
        if (!i) return "/";
    for (; path[i] != '/'; i--)
        if (!i) return ".";
    for (; path[i] == '/'; i--)
        if (!i) return "/";
    path[i + 1] = 0;
    return path;
}

static char *basename(char const *path)
{
    char *p = strrchr(path, '/');
    return p ? p + 1 : (char *)path;
}
