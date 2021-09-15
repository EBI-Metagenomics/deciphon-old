#include "cli.h"
#include "dcp/cli.h"
#include "dcp/generics.h"
#include "dcp/pro_db.h"
#include "dcp/pro_reader.h"
#include "error.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define _POSIX_C_SOURCE 1
#include <limits.h>

struct arguments
{
    char *args[1];
    int quiet;
    char const *input_file;
    char output_file[PATH_MAX];
};

static bool infer_output_file(struct arguments *args)
{
    size_t n = strlcpy(args->output_file, args->input_file, PATH_MAX);
    if (n >= PATH_MAX) return false;
    return cli_change_or_add_ext(args->output_file, PATH_MAX, ".dcp");
}

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *args = state->input;

    switch (key)
    {
    case 'q':
        args->quiet = 1;
        break;

    case 'o':
        if (strlcpy(args->output_file, arg, PATH_MAX) >= PATH_MAX)
        {
            error(DCP_ILLEGALARG, "output path is too long");
            return ENAMETOOLONG;
        }
        break;

    case ARGP_KEY_ARG:
        if (state->arg_num >= 1) argp_usage(state);
        args->args[state->arg_num] = arg;
        args->input_file = arg;
        break;

    case ARGP_KEY_END:
        if (state->arg_num < 1)
        {
            argp_usage(state);
        }
        else
        {
            if (!args->output_file[0] && !infer_output_file(args))
            {
                error(DCP_ILLEGALARG, "output path would be too long");
                return ENAMETOOLONG;
            }
        }
        break;

    case ARGP_KEY_FINI:
    case ARGP_KEY_SUCCESS:
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static char doc[] = "Press a HMMER3 file -- dcp-press file.hmm";
static char args_doc[] = "HMM";
static struct argp_option options[] = {
    {"quiet", 'q', 0, 0, "Disable output", 0},
    {"output", 'o', "FILE", 0, "Output to FILE instead of standard output", 0},
    {0}};
static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

static struct
{
    struct
    {
        char const *file;
        FILE *fd;
    } input;
    struct
    {
        char const *file;
        FILE *fd;
    } output;

    struct
    {
        struct athr *at;
        long pos;
    } progress;
    struct dcp_pro_db db;
    struct dcp_pro_reader reader;
} cli = {0};

static off_t filesize(void)
{
    int fd = fileno(cli.input.fd);
    struct stat st;
    fstat(fd, &st);
    return st.st_size;
}

static void progress_init(void)
{
    cli.progress.pos = 0;
    cli.progress.at = NULL;
}

static void progress_start(void)
{
    cli.progress.pos = ftell(cli.input.fd);
    cli.progress.at =
        athr_create(filesize() - cli.progress.pos, "Press", ATHR_BAR);
}

static void progress_update(void)
{
    athr_consume(cli.progress.at, ftell(cli.input.fd) - cli.progress.pos);
    cli.progress.pos = ftell(cli.input.fd);
}

static void progress_end(void)
{
    if (cli.progress.at) athr_finish(cli.progress.at);
}

static enum dcp_rc cli_setup(struct arguments const *args)
{
    cli_log_setup();
    cli.input.file = args->input_file;
    cli.output.file = args->output_file;

    if (!(cli.input.fd = fopen(cli.input.file, "r")))
        return error(DCP_IOERROR, "failed to open the hmm file");

    if (!(cli.output.fd = fopen(cli.output.file, "wb")))
        return error(DCP_IOERROR, "failed to open the output file");

    progress_init();

    cli.db = dcp_pro_db_default;

    enum dcp_rc rc =
        dcp_pro_db_openw(&cli.db, cli.output.fd, &imm_amino_iupac,
                         imm_super(&imm_dna_iupac), DCP_PRO_CFG_DEFAULT);
    if (rc) return rc;

    dcp_pro_reader_init(&cli.reader, &cli.db.amino, &cli.db.nuclt,
                        cli.db.prof.cfg, cli.input.fd);

    return DCP_SUCCESS;
}

static void cli_end(enum dcp_rc rc)
{
    if (!rc) progress_end();
    fclose(cli.input.fd);
    fclose(cli.output.fd);
    cli_log_flush();
}

static enum dcp_rc profile_write(void)
{
    dcp_prof_nameit(dcp_super(&cli.db.prof), dcp_pro_reader_meta(&cli.reader));

    enum dcp_rc rc = dcp_pro_prof_absorb(&cli.db.prof, &cli.reader.model);
    if (rc) return rc;

    return dcp_pro_db_write(&cli.db, &cli.db.prof);
}

enum dcp_rc dcp_cli_press(int argc, char **argv)
{
    struct arguments arguments = {0};
    if (argp_parse(&argp, argc, argv, 0, 0, &arguments)) return DCP_ILLEGALARG;

    enum dcp_rc rc = cli_setup(&arguments);
    if (rc) goto cleanup;

    progress_start();
    while (!(rc = dcp_pro_reader_next(&cli.reader)))
    {
        progress_update();
        rc = profile_write();
    }
    if (rc != DCP_END)
    {
        error(rc, "failed to parse HMM file");
        goto cleanup;
    }

    rc = dcp_pro_db_close(&cli.db);

cleanup:
    cli_end(rc);
    return rc;
}
