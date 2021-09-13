#include "cli.h"
#include "dcp/cli.h"
#include "dcp/generics.h"
#include "dcp/pro_db.h"
#include "dcp/pro_reader.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

char const *argp_program_version = "dcp-press " DCP_VERSION;
char const *argp_program_bug_address = CLI_BUG_ADDRESS;

struct arguments
{
    char *args[2];
} arguments;

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *args = state->input;

    switch (key)
    {
    case ARGP_KEY_ARG:
        if (state->arg_num >= 2) argp_usage(state);
        args->args[state->arg_num] = arg;
        break;

    case ARGP_KEY_END:
        if (state->arg_num < 2) argp_usage(state);
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static char doc[] = "Press a HMMER3 file -- dcp-press file.hmm file.dcp";
static char args_doc[] = "HMM DCP";
static struct argp_option options[] = {{0}};
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

static void progress_end(void) { athr_finish(cli.progress.at); }

static enum dcp_rc cli_setup(void)
{
    cli_log_setup();
    cli.input.file = arguments.args[0];
    cli.output.file = arguments.args[1];
    cli.input.fd = fopen(cli.input.file, "r");
    cli.output.fd = fopen(cli.output.file, "wb");

    progress_start();

    cli.db = dcp_pro_db_default;

    enum dcp_rc rc =
        dcp_pro_db_openw(&cli.db, cli.output.fd, &imm_amino_iupac,
                         imm_super(&imm_dna_iupac), DCP_PRO_CFG_DEFAULT);
    if (rc) return rc;

    dcp_pro_reader_init(&cli.reader, &cli.db.amino, &cli.db.nuclt,
                        cli.db.prof.cfg, cli.input.fd);

    return DCP_SUCCESS;
}

static void cli_end(void)
{
    progress_end();
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
    if (argp_parse(&argp, argc, argv, 0, 0, &arguments)) return DCP_ILLEGALARG;

    enum dcp_rc rc = cli_setup();
    while (!(rc = dcp_pro_reader_next(&cli.reader)))
    {
        progress_update();
        rc = profile_write();
    }
    if (rc != DCP_END) goto cleanup;

    rc = dcp_pro_db_close(&cli.db);

cleanup:
    cli_end();
    return rc;
}
