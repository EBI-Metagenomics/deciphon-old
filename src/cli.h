#ifndef CLI_H
#define CLI_H

#include "athr/athr.h"
#include "dcp/version.h"
#include <argp.h>
#include <stdbool.h>
#include <stdio.h>

struct cli_progress
{
    FILE *fd;
    long position;
    struct athr *at;
};

void cli_log_setup(void);
void cli_log_flush(void);

#define CLI_BUG_ADDRESS "<https://github.com/EBI-Metagenomics/deciphon>"

#endif
