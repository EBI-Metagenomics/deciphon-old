#ifndef CLI_H
#define CLI_H

#include "common/rc.h"

#define CLI_BUG_ADDRESS "<https://github.com/EBI-Metagenomics/deciphon>"

void cli_logger_setup(void);
void cli_logger_flush(void);

enum rc cli_info(int argc, char **argv);
enum rc cli_press(int argc, char **argv);
enum rc cli_scan(int argc, char **argv);
enum rc cli_server(int argc, char **argv);

#endif
