#include "core/command_help.h"
#include <stdio.h>

static char table[1024] = {0};
static char *pos = table;

void command_help_init(void)
{
    table[0] = '\0';
    pos = table;
    pos += sprintf(pos, "Commands:");
}

void command_help_add(char const *name, char const *params)
{
    pos += sprintf(pos, "\n  %-22s %s", name, params);
}

char const *command_help_table(void) { return table; }
