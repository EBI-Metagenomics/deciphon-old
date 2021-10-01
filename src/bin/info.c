#include "cli.h"
#include "dcp/cli.h"

char const *argp_program_version = "dcp-info " DCP_VERSION;
char const *argp_program_bug_address = CLI_BUG_ADDRESS;

int main(int argc, char **argv) { return (int)dcp_cli_info(argc, argv); }
