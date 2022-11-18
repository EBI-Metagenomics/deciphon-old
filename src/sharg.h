#ifndef SHARG_H
#define SHARG_H

#include <stdbool.h>

enum
{
    SHARG_ARGV_SIZE = 32,
    SHARG_BUFF_SIZE = 4096,
};

struct sharg
{
    int argc;
    char *argv[SHARG_ARGV_SIZE];
    char delim;
    char buff[SHARG_BUFF_SIZE];
};

void sharg_init(struct sharg *);
bool sharg_parse(struct sharg *, char const *str);
char sharg_delim(struct sharg const *);
int sharg_argc(struct sharg const *);
char **sharg_argv(struct sharg *);
char const *sharg_shift(struct sharg *);
void sharg_append(struct sharg *, char const *str);
bool sharg_replace(struct sharg *, char const *tag, char const *value);
bool sharg_check(struct sharg const *, char const *fmt);
char *sharg_unparse(struct sharg *sharg);

#endif
