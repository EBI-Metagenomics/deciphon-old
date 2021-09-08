#include "cli.h"
#include "dcp/cli.h"
#include "dcp/generics.h"
#include "dcp/pro_db.h"
#include "dcp/pro_reader.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const char *argp_program_version = "dcp-press " DCP_VERSION;
const char *argp_program_bug_address = CLI_BUG_ADDRESS;

static char doc[] = "Press a HMMER3 file -- dcp-press file.hmm file.dcp";

static char args_doc[] = "HMM DCP";

static struct argp_option options[] = {{0}};

struct arguments
{
    char *args[2];
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;

    switch (key)
    {
    case ARGP_KEY_ARG:
        if (state->arg_num >= 2) argp_usage(state);

        arguments->args[state->arg_num] = arg;

        break;

    case ARGP_KEY_END:
        if (state->arg_num < 2) argp_usage(state);
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

struct faa
{
    char const *filepath;
    FILE *fd;
};

struct dcp
{
    char const *filepath;
    FILE *fd;
};

static enum dcp_rc openw_db(struct dcp_pro_db *db, FILE *fd,
                            struct dcp_pro_cfg cfg)
{
    struct imm_amino const *amino = &imm_amino_iupac;
    struct imm_nuclt const *nuclt = imm_super(&imm_dna_iupac);
    dcp_pro_db_init(db);
    return dcp_pro_db_openw(db, fd, amino, nuclt, cfg);
}

static off_t filesize(FILE *restrict f)
{
    int fd = fileno(f);
    struct stat st;
    fstat(fd, &st);
    return st.st_size;
}

enum dcp_rc dcp_cli_press(int argc, char **argv)
{
    struct arguments arguments;
    if (argp_parse(&argp, argc, argv, 0, 0, &arguments)) return DCP_ILLEGALARG;

    cli_setup();

    struct faa iput = {arguments.args[0], NULL};
    struct dcp oput = {arguments.args[1], NULL};

    iput.fd = fopen(iput.filepath, "r");
    oput.fd = fopen(oput.filepath, "wb");

    enum dcp_rc rc = DCP_SUCCESS;

    struct dcp_pro_db db;
    dcp_pro_db_init(&db);

    if ((rc = openw_db(&db, oput.fd, DCP_PRO_CFG_DEFAULT))) goto cleanup;

    struct dcp_pro_reader reader;
    dcp_pro_reader_init(&reader, &db.amino, &db.nuclt, db.prof.cfg, iput.fd);

    long pos = ftell(iput.fd);
    struct athr *at = athr_create(filesize(iput.fd) - pos, "Press", ATHR_BAR);
    while (!(rc = dcp_pro_reader_next(&reader)))
    {
        athr_consume(at, ftell(iput.fd) - pos);
        dcp_prof_nameit(dcp_super(&db.prof), dcp_pro_reader_meta(&reader));
        if ((rc = dcp_pro_prof_absorb(&db.prof, &reader.model))) goto cleanup;
        if ((rc = dcp_pro_db_write(&db, &db.prof))) goto cleanup;
        pos = ftell(iput.fd);
    }
    if (rc != DCP_ENDFILE) goto cleanup;

    rc = dcp_pro_db_close(&db);
    athr_finish(at);

cleanup:
    fclose(iput.fd);
    fclose(oput.fd);
    cli_end();
    return rc;
}
