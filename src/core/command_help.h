#ifndef CORE_COMMAND_HELP_H
#define CORE_COMMAND_HELP_H

void command_help_init(void);
void command_help_add(char const *name, char const *params);
char const *command_help_table(void);

#endif
