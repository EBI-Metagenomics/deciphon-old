#include "cli.h"
#include "dcp/cli.h"
#include "imm/imm.h"
#include <stdio.h>

const char *argp_program_version = "dcp-info " DCP_VERSION;
const char *argp_program_bug_address = CLI_BUG_ADDRESS;

struct arguments
{
    char *args[0];
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *args = state->input;

    switch (key)
    {
    case ARGP_KEY_ARG:
        if (state->arg_num >= 0) argp_usage(state);

        args->args[state->arg_num] = arg;

        break;

    case ARGP_KEY_END:
        if (state->arg_num < 0) argp_usage(state);
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static char doc[] = "Show information -- dcp-info";
static char args_doc[] = "";
static struct argp_option options[] = {{0}};
static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

enum dcp_rc dcp_cli_info(int argc, char **argv)
{
    struct arguments arguments;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);
    printf("Version: " DCP_VERSION "\n");
    printf("Float size: %d\n", IMM_FLOAT_BYTES);
    return DCP_SUCCESS;
}
