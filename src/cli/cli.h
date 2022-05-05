#ifndef CLI_H
#define CLI_H

#include "deciphon/core/rc.h"

void cli_logger_setup(void);
void cli_logger_flush(void);

enum rc cli_server(void);

#endif
