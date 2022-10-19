#include "core/msg.h"
#include "core/fmt.h"
#include "core/logy.h"
#include "core/rc.h"
#include <stdarg.h>
#include <string.h>

void msg_init(struct msg *msg)
{
    sharg_init(&msg->cmd);
    sharg_init(&msg->ctx);
}

static int find_pipe(char const *str, char delim);

int msg_parse(struct msg *msg, char *str)
{
    sharg_init(&msg->cmd);
    sharg_init(&msg->ctx);

    if (!sharg_parse(&msg->cmd, str)) return eparse("invalid message");

    char delim = sharg_delim(&msg->cmd);
    int pipe = find_pipe(str, delim);
    if (pipe < 0) return RC_OK;
    int n = (int)strlen(str);

    char *cmd_start = str;
    *(str + pipe - (pipe != 0)) = '\0';

    char *ctx_start = str + pipe + 1 + (pipe + 1 != n);

    if (!sharg_parse(&msg->cmd, cmd_start)) return eparse("invalid message");
    if (!sharg_parse(&msg->ctx, ctx_start)) return eparse("invalid message");

    return RC_OK;
}

char *msg_unparse(struct msg *msg)
{
    if (msg->ctx.argc > 0) sharg_append(&msg->cmd, "|");
    for (int i = 0; i < msg->ctx.argc; ++i)
        sharg_append(&msg->cmd, msg->ctx.argv[i]);
    return sharg_unparse(&msg->cmd);
}

int msg_check(struct msg const *msg, char const *fmt)
{
    return sharg_check(&msg->cmd, fmt) ? RC_OK
                                       : einval("invalid message format");
}

char const *msg_cmd(struct msg const *msg) { return msg->cmd.argv[0]; }

static void ctx_set(struct msg *msg, char const *tag, char const *value)
{
    sharg_replace(&msg->ctx, tag, value);
}

static char *ctx_unparse(struct msg *msg) { return sharg_unparse(&msg->ctx); }

char *_msg_ctx(struct msg *msg, int nargs, ...)
{
    va_list valist;
    va_start(valist, nargs);
    for (int i = 1; i <= nargs; ++i)
        ctx_set(msg, fmt("{%i}", i), va_arg(valist, char *));
    va_end(valist);
    return ctx_unparse(msg);
}

static int find_pipe(char const *str, char delim)
{
    int size = (int)strlen(str);
    if (size == 0) return -1;
    if (str[0] != '|' && size == 1) return -1;
    if (str[0] == '|' && size == 1) return 0;
    if (str[0] == '|' && size > 1 && str[1] == delim) return 0;

    char const *p = str;
    while ((p = strchr(p, '|')))
    {
        char const *prev = p - 1;
        char const *next = p + 1;
        if (*prev == delim && *next == delim) return (int)(p - str);
        ++p;
    }
    return -1;
}
