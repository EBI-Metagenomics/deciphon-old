#ifndef ARGLESS_H
#define ARGLESS_H

#include <stdbool.h>

struct argl_option;

struct argl
{
    struct argl_option const *options;
    char const *args_doc;
    char const *doc;
    char const *version;

    int argc;
    char **argv;
};

void argl_parse(struct argl *, int argc, char *argv[]);
bool argl_has(struct argl const *, char const *name);
char const *argl_get(struct argl const *, char const *name);
char const *argl_grab(struct argl const *, char const *name, char const *value);
int argl_nargs(struct argl const *);
char **argl_args(struct argl const *);
void argl_usage(struct argl const *);
char const *argl_program(struct argl const *);

enum
{
    ARGL_NOVALUE = 0,
    ARGL_HASVALUE = 1,
};

struct argl_option
{
    char const *name;
    char const key;
    char const *arg_name;
    char const *arg_doc;
    bool const has_value;
};

#ifndef NULL
#define NULL ((void *)0)
#endif

#define ARGL_HELP_KEY '?'
#define ARGL_USAGE_KEY -1
#define ARGL_VERSION_KEY 'V'

#define ARGL_HELP_OPT                                                          \
    {                                                                          \
        "help", ARGL_HELP_KEY, NULL, "Give this help list", ARGL_NOVALUE       \
    }
#define ARGL_USAGE_OPT                                                         \
    {                                                                          \
        "usage", ARGL_USAGE_KEY, NULL, "Give a short usage message",           \
            ARGL_NOVALUE                                                       \
    }

#define ARGL_VERSION_OPT                                                       \
    {                                                                          \
        "version", ARGL_VERSION_KEY, NULL, "Print program version",            \
            ARGL_NOVALUE                                                       \
    }

#define ARGL_NULL_OPT                                                          \
    {                                                                          \
        0, 0, 0, 0, 0                                                          \
    }

#define ARGL_DEFAULT_OPTS ARGL_HELP_OPT, ARGL_USAGE_OPT, ARGL_VERSION_OPT

#endif
