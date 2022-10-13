#ifndef CORE_MSG_H
#define CORE_MSG_H

#include "sharg.h"

struct msg
{
    struct sharg cmd;
    struct sharg echo;
};

void msg_init(struct msg *);
bool msg_parse(struct msg *, char *str);

#endif
