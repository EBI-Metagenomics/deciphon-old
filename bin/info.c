#include "cli.h"
#include "cli_info.h"
#include "version.h"

char const *argp_program_version = "dcp-info " VERSION;
char const *argp_program_bug_address = CLI_BUG_ADDRESS;

int main(int argc, char **argv) { return (int)cli_info(argc, argv); }
