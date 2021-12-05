#include "cli.h"
#include "version.h"

char const *argp_program_version = "dcp-press " VERSION;
char const *argp_program_bug_address = CLI_BUG_ADDRESS;

int main(int argc, char **argv) { return (int)cli_press(argc, argv); }
