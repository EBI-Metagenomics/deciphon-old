#include "cli.h"
#include "cli_server.h"
#include "version.h"

char const *argp_program_version = "deciphond " VERSION;
char const *argp_program_bug_address = CLI_BUG_ADDRESS;

int main(int argc, char **argv) { return (int)cli_server(argc, argv); }
