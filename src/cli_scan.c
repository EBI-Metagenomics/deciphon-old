#include "cli.h"
#include "dcp/cli.h"
#include "dcp/generics.h"
#include "dcp/pro_codec.h"
#include "dcp/pro_db.h"
#include "dcp/pro_reader.h"
#include "dcp/pro_state.h"
#include "error.h"
#include "fasta/fasta.h"
#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

char const *argp_program_version = "dcp-scan " DCP_VERSION;
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

static char doc[] = "Scan a target -- dcp-scan targets.faa db.dcp";
static char args_doc[] = "FAA DCP";
static struct argp_option options[] = {{0}};
static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

static struct
{
    struct
    {
        char const *file;
        FILE *fd;
        struct fasta fa;
    } targets;

    struct
    {
        struct
        {
            char const *file;
            FILE *fd;
            struct fasta fa;
            char seq[FASTA_SEQ_MAX];
        } codon;
        struct
        {
            char const *file;
            FILE *fd;
            struct fasta fa;
            char seq[FASTA_SEQ_MAX];
        } amino;
    } output;

    struct
    {
        char const *file;
        FILE *fd;
    } db;

} cli = {0};

static char const default_ocodon[] = "ocodon.fa";
static char const default_oamino[] = "oamino.fa";

static enum dcp_rc scan_targets(struct imm_result *result,
                                struct dcp_pro_prof *prof)
{
    struct imm_task *task = imm_task_new(&prof->alt.dp);
    struct imm_abc const *abc = &prof->nuclt->super;

    enum fasta_rc fasta_rc = FASTA_SUCCESS;
    while (!(fasta_rc = fasta_read(&cli.targets.fa)))
    {
        char const *tgt = cli.targets.fa.target.seq;
        struct imm_seq seq = imm_seq(imm_str(tgt), abc);
        if (imm_task_setup(task, &seq))
            return error(DCP_RUNTIMEERROR, "failed to create task");

        if (imm_dp_viterbi(&prof->alt.dp, task, result))
            return error(DCP_RUNTIMEERROR, "failed to run viterbi");

        struct imm_path const *path = &result->path;
        struct dcp_pro_codec codec = dcp_pro_codec_init(prof, path);
        unsigned any = imm_abc_any_symbol_id(abc);
        struct imm_codon codon = imm_codon(prof->nuclt, any, any, any);
        enum dcp_rc rc = DCP_SUCCESS;
        char *ocodon = cli.output.codon.seq;
        struct fasta_target tcodon = {cli.targets.fa.target.id,
                                      cli.targets.fa.target.desc, ocodon};
        while (!(rc = dcp_pro_codec_next(&codec, &seq, &codon)))
        {
            *(ocodon++) = imm_codon_asym(&codon);
            *(ocodon++) = imm_codon_bsym(&codon);
            *(ocodon++) = imm_codon_csym(&codon);
        }
        *ocodon = '\0';
        fasta_write(&cli.output.codon.fa, tcodon, 60);
        /* printf("%s: %f\n", cli.targets.fa.target.id, result->loglik); */
    }

    imm_del(task);
    return DCP_SUCCESS;
}

static void setup_arguments(void)
{
    cli.targets.file = arguments.args[0];
    cli.db.file = arguments.args[1];
}

static void setup_files(void)
{
    cli.targets.fd = fopen(cli.targets.file, "r");
    cli.db.fd = fopen(cli.db.file, "rb");

    cli.output.codon.file = default_ocodon;
    cli.output.amino.file = default_oamino;

    cli.output.codon.fd = fopen(cli.output.codon.file, "w");
    cli.output.amino.fd = fopen(cli.output.amino.file, "w");

    fasta_init(&cli.output.codon.fa, cli.output.codon.fd, FASTA_WRITE);
    fasta_init(&cli.output.amino.fa, cli.output.amino.fd, FASTA_WRITE);

    cli.output.codon.seq[0] = '\0';
    cli.output.amino.seq[0] = '\0';
}

enum dcp_rc dcp_cli_scan(int argc, char **argv)
{
    if (argp_parse(&argp, argc, argv, 0, 0, &arguments)) return DCP_ILLEGALARG;

    cli_log_setup();
    setup_arguments();
    setup_files();

    enum dcp_rc rc = DCP_SUCCESS;

    struct dcp_pro_db db = dcp_pro_db_default;

    if ((rc = dcp_pro_db_openr(&db, cli.db.fd))) goto cleanup;

    struct dcp_pro_prof *prof = dcp_pro_db_profile(&db);

    assert(imm_abc_typeid(&db.nuclt.super) == IMM_DNA);

    struct imm_result result = imm_result();
    while (!(rc = dcp_db_end(dcp_super(&db))))
    {
        if ((rc = dcp_pro_db_read(&db, prof))) goto cleanup;
        assert(dcp_prof_typeid(dcp_super(prof)) == DCP_PROTEIN_PROFILE);

        fasta_init(&cli.targets.fa, cli.targets.fd, FASTA_READ);
        rewind(cli.targets.fd);
        if ((rc = scan_targets(&result, prof))) goto cleanup;
    }
    if (rc != DCP_END) goto cleanup;

    rc = dcp_pro_db_close(&db);
    imm_del(&result);

cleanup:
    fclose(cli.targets.fd);
    fclose(cli.db.fd);
    fclose(cli.output.codon.fd);
    fclose(cli.output.amino.fd);
    cli_log_flush();
    return rc;
}
