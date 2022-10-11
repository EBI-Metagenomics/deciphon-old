#ifndef DECY_SESSION_H
#define DECY_SESSION_H

#include <stdbool.h>

struct cmd;

enum proc_id
{
    PRESSY_ID = 0,
    SCANNY_ID = 1,
    SCHEDY_ID = 2,
};

void session_init(void);
char const *session_forward_command(char const *proc_name, struct cmd *cmd);

#endif
