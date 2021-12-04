#include "cli.h"
#include "cli_scan.h"
#include "version.h"

char const *argp_program_version = "dcp-scan " VERSION;
char const *argp_program_bug_address = CLI_BUG_ADDRESS;

int main(int argc, char **argv) { return (int)cli_scan(argc, argv); }
