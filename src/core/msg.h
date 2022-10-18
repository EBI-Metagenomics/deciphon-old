#ifndef CORE_MSG_H
#define CORE_MSG_H

#include "sharg.h"

struct msg
{
    struct sharg cmd;
    struct sharg ctx;
};

typedef void msg_fn_t(struct msg *);

void msg_init(struct msg *);
bool msg_parse(struct msg *, char *str);
char *msg_unparse(struct msg *);

#endif
