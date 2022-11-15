#ifndef CORE_CMD_H
#define CORE_CMD_H

struct msg;

typedef void cmd_fn_t(struct msg *);

struct cmd_entry
{
    cmd_fn_t *func;
    char const *name;
    char const *doc;
};

struct cmd_entry *cmd_find(int size, struct cmd_entry[], char const *name);

void cmd_help_init(void);
void cmd_help_add(char const *name, char const *params);
char const *cmd_help_table(void);

#endif
