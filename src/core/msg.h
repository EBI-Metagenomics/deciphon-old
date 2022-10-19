#ifndef CORE_MSG_H
#define CORE_MSG_H

#include "core/pp.h"
#include "core/sharg.h"
#include <stdint.h>

struct msg
{
    struct sharg cmd;
    struct sharg ctx;
};

void msg_init(struct msg *);
int msg_parse(struct msg *, char *str);
char *msg_unparse(struct msg *);
int msg_check(struct msg const *, char const *fmt);
int msg_argc(struct msg const *);
char **msg_argv(struct msg *);
char const *msg_cmd(struct msg const *);
char const *msg_str(struct msg const *, int idx);
int64_t msg_int(struct msg const *, int idx);
char *_msg_ctx(struct msg *, int nargs, ...);
#define msg_ctx(msg, ...) _msg_ctx(msg, PP_NARG(__VA_ARGS__), __VA_ARGS__)

#endif
