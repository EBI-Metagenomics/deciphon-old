#ifndef DCP_CLI_H
#define DCP_CLI_H

#include "dcp/export.h"
#include "dcp/rc.h"

DCP_API enum dcp_rc dcp_cli_info(int argc, char **argv);
DCP_API enum dcp_rc dcp_cli_press(int argc, char **argv);
DCP_API enum dcp_rc dcp_cli_scan(int argc, char **argv);

#endif
