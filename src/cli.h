#ifndef CLI_H
#define CLI_H

#include "athr/athr.h"
#include "dcp/version.h"
#include <argp.h>
#include <stdbool.h>

void cli_log_setup(void);
void cli_log_flush(void);
bool cli_change_or_add_ext(char *str, size_t max_size, char const *ext);

#define CLI_BUG_ADDRESS "<https://github.com/EBI-Metagenomics/deciphon>"

#endif
