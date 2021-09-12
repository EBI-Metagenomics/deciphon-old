#include "cli.h"
#include "dcp/cli.h"
#include "dcp/generics.h"
#include "dcp/pro_codec.h"
#include "dcp/pro_db.h"
#include "dcp/pro_reader.h"
#include "dcp/pro_state.h"
#include "error.h"
#include "far/far.h"
#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const char *argp_program_version = "dcp-scan " DCP_VERSION;
const char *argp_program_bug_address = CLI_BUG_ADDRESS;

static char doc[] = "Scan a target -- dcp-scan targets.faa db.dcp";

static char args_doc[] = "FAA DCP";

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

static off_t filesize(FILE *restrict f)
{
    int fd = fileno(f);
    struct stat st;
    fstat(fd, &st);
    return st.st_size;
}

static struct far far;
static struct far_tgt tgt;

static enum dcp_rc scan_targets(FILE *fd, struct imm_result *result,
                                struct imm_dp *dp, struct dcp_pro_prof *prof)
{
    struct imm_task *task = imm_task_new(dp);
    assert(fd);

    far_init(&far, fd);
    far_tgt_init(&tgt, &far);

    enum far_rc far_rc = FAR_SUCCESS;
    while (!(far_rc = far_next_tgt(&far, &tgt)))
    {
        struct imm_seq seq = imm_seq(imm_str(tgt.seq), &prof->nuclt->super);
        if (imm_task_setup(task, &seq))
            return error(DCP_RUNTIMEERROR, "failed to create task");

        if (imm_dp_viterbi(dp, task, result))
            return error(DCP_RUNTIMEERROR, "failed to run viterbi");

        struct imm_path const *path = &result->path;
        struct dcp_pro_codec codec = dcp_pro_codec_init(prof, path);
        unsigned any = imm_abc_any_symbol_id(&prof->nuclt->super);
        struct imm_codon codon = imm_codon(prof->nuclt, any, any, any);
        enum dcp_rc rc = DCP_SUCCESS;
        while (!(rc = dcp_pro_codec_next(&codec, &seq, &codon)))
        {
            printf("%c%c%c", imm_codon_asym(&codon), imm_codon_bsym(&codon),
                   imm_codon_csym(&codon));
        }
        printf("\n");

        printf("%s: %f\n", tgt.id, result->loglik);
    }

    imm_del(task);
    return DCP_SUCCESS;
}

enum dcp_rc dcp_cli_scan(int argc, char **argv)
{
    struct arguments arguments;
    if (argp_parse(&argp, argc, argv, 0, 0, &arguments)) return DCP_ILLEGALARG;

    cli_setup();

    struct faa faa = {arguments.args[0], NULL};
    struct dcp dcp = {arguments.args[1], NULL};

    faa.fd = fopen(faa.filepath, "r");
    dcp.fd = fopen(dcp.filepath, "rb");

    enum dcp_rc rc = DCP_SUCCESS;

    struct dcp_pro_db db;
    dcp_pro_db_init(&db);

    if ((rc = dcp_pro_db_openr(&db, dcp.fd))) goto cleanup;
    struct dcp_pro_prof *prof = dcp_pro_db_profile(&db);

    struct imm_nuclt const *nuclt = dcp_pro_db_nuclt(&db);
    assert(imm_abc_typeid(imm_super(nuclt)) == IMM_DNA);

    /* long pos = ftell(dcp.fd); */
    /* struct athr *at = athr_create(filesize(dcp.fd) - pos, "Scan", ATHR_BAR);
     */
    struct imm_result result = imm_result();
    while (!(rc = dcp_db_end(dcp_super(&db))))
    {
        /* athr_consume(at, ftell(dcp.fd) - pos); */
        if ((rc = dcp_pro_db_read(&db, prof))) goto cleanup;
        assert(dcp_prof_typeid(dcp_super(prof)) == DCP_PROTEIN_PROFILE);

        if ((rc = scan_targets(faa.fd, &result, &prof->alt.dp, prof)))
            goto cleanup;

        /* pos = ftell(dcp.fd); */
    }
    if (rc != DCP_END) goto cleanup;

    rc = dcp_pro_db_close(&db);
    imm_del(&result);
    /* athr_finish(at); */

cleanup:
    fclose(faa.fd);
    fclose(dcp.fd);
    cli_end();
    return rc;
}
