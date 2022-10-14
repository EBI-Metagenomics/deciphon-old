#include "core/msg.h"
#include <string.h>

void msg_init(struct msg *msg)
{
    sharg_init(&msg->cmd);
    sharg_init(&msg->ctx);
}

static int find_pipe(char const *str, char delim);

bool msg_parse(struct msg *msg, char *str)
{
    sharg_init(&msg->cmd);
    sharg_init(&msg->ctx);

    if (!sharg_parse(&msg->cmd, str)) return false;

    char delim = sharg_delim(&msg->cmd);
    int pipe = find_pipe(str, delim);
    if (pipe < 0) return true;
    int n = (int)strlen(str);

    char *cmd_start = str;
    *(str + pipe - (pipe != 0)) = '\0';

    char *echo_start = str + pipe + 1 + (pipe + 1 != n);

    if (!sharg_parse(&msg->cmd, cmd_start)) return false;
    if (!sharg_parse(&msg->ctx, echo_start)) return false;

    return true;
}

char *msg_unparse(struct msg *msg)
{
    for (int i = 0; i < msg->ctx.argc; ++i)
        sharg_append(&msg->cmd, msg->ctx.argv[i]);
    return sharg_unparse(&msg->cmd);
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
