#include "cli.h"
#include "imm/imm.h"
#include "rc.h"
#include "version.h"
#include <argp.h>

struct arguments
{
    char *args[0];
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    switch (key)
    {
    case ARGP_KEY_ARG:
        argp_usage(state);
        break;

    case ARGP_KEY_END:
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

enum rc cli_info(int argc, char **argv)
{
    struct arguments arguments;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);
    printf("Version: " VERSION "\n");
    printf("Float size: %d\n", IMM_FLOAT_BYTES);
    return RC_DONE;
}