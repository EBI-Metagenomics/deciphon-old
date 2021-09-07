#include "athr/athr.h"
#include "dcp/dcp.h"
#include "log/log.h"
#include <argp.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const char *argp_program_version = "dcp-press " DCP_VERSION;
const char *argp_program_bug_address =
    "<https://github.com/EBI-Metagenomics/deciphon>";

static char doc[] = "Press a HMMER3 file -- dcp-press file.hmm file.dcp";

static char args_doc[] = "HMM DCP";

static struct argp_option options[] = {{0}};

struct arguments
{
    char *args[2];
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    /* Get the input argument from argp_parse, which we
       know is a pointer to our arguments structure. */
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

static struct argp argp = {options, parse_opt, args_doc, doc};

struct input
{
    char const *filepath;
    FILE *fd;
};

struct output
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

static void wrap_log_put(char const *msg, void *arg)
{
    __log_put(LOG_ERROR, msg);
}

static void wrap_fflush(void *arg)
{
    FILE *fd = arg;
    fflush(fd);
}

static void wrap_fprintf(char const *msg, void *arg)
{
    fprintf(stderr, "%s\n", msg);
}

off_t filesize(FILE *restrict f)
{
    int fd = fileno(f);
    struct stat st;
    fstat(fd, &st);
    return st.st_size;
}

int main(int argc, char **argv)
{
    struct arguments arguments;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    imm_log_setup(wrap_log_put, NULL);
    dcp_log_setup(wrap_log_put, NULL);
    log_setup(LOG_ERROR, wrap_fprintf, wrap_fflush, NULL);

    struct input iput = {arguments.args[0], NULL};
    struct output oput = {arguments.args[1], NULL};

    iput.fd = fopen(iput.filepath, "r");
    oput.fd = fopen(oput.filepath, "wb");

    struct dcp_pro_db db;
    openw_db(&db, oput.fd, dcp_pro_cfg(DCP_ENTRY_DIST_OCCUPANCY, 0.1f));

    struct dcp_pro_reader reader;
    dcp_pro_reader_init(&reader, &db.amino, &db.nuclt, db.prof.cfg, iput.fd);

    long pos = ftell(iput.fd);
    struct athr *at = athr_create(filesize(iput.fd) - pos);
    while (!dcp_pro_reader_next(&reader))
    {
        athr_consume(at, ftell(iput.fd) - pos);
        dcp_pro_prof_absorb(&db.prof, &reader.model);
        dcp_pro_db_write(&db, &db.prof);
        pos = ftell(iput.fd);
    }
    athr_finish(at);

    fclose(iput.fd);
    fclose(oput.fd);

    log_flush();
    exit(0);
}
