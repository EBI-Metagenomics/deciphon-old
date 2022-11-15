#include "core/cmd.h"
#include <stdio.h>
#include <string.h>

struct cmd_entry *cmd_find(int size, struct cmd_entry entries[],
                           char const *name)
{
    int i = 0;
    while (i < size && strcmp(name, entries[i].name))
        ++i;

    if (i == size) return NULL;
    return entries + i;
}

static char table[1024] = {0};
static char *pos = table;

void cmd_help_init(void)
{
    table[0] = '\0';
    pos = table;
    pos += sprintf(pos, "Commands:");
}

void cmd_help_add(char const *name, char const *params)
{
    pos += sprintf(pos, "\n  %-22s %s", name, params);
}

char const *cmd_help_table(void) { return table; }
