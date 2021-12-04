#include "cli_scan.h"
#include "cli.h"
#include "fasta/fasta.h"
#include "gff/gff.h"
#include "imm/imm.h"
#include "logger.h"
#include "progress_file.h"
#include "protein_codec.h"
#include "protein_db.h"
#include "protein_reader.h"
#include "protein_state.h"
#include "table.h"
#include "tbl/tbl.h"
#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct arguments
{
    char *args[2];
    int quiet;
} arguments;

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *args = state->input;

    switch (key)
    {
    case 'q':
        args->quiet = 1;
        break;

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

static char doc[] = "Scan queries -- dcp-scan queries.fna pfam.dcp";
static char args_doc[] = "FNA DCP";
static struct argp_option options[] = {
    {"quiet", 'q', 0, 0, "Disable output", 0}, {0}};
static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

static struct
{
    struct
    {
        char const *file;
        FILE *fd;
        struct fasta fa;
    } queries;

    struct
    {
        char const *file;
        FILE *fd;
        struct protein_db db;
        struct imm_prod prod;
    } pro;

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

        char const *file;
        FILE *fd;
        struct gff gff;

        unsigned nmatches;
    } output;

    struct progress_file progress;
} cli = {0};

static char const default_ocodon[] = "codon.fna";
static char const default_oamino[] = "amino.fa";
static char const default_output[] = "output.gff";

static struct imm_seq seq_setup(void)
{
    char const *seq = cli.queries.fa.target.seq;
    struct protein_profile *prof = &cli.pro.db.prof;
    struct imm_abc const *abc = &prof->code->nuclt->super;
    return imm_seq(imm_str(seq), abc);
}

static struct tbl_8x_ed table = {0};

static void annotate(struct imm_seq const *sequence, char const *profile_name,
                     char const *seq_name)
{
    char const *headers[4] = {profile_name, seq_name, "posterior", ""};
    tbl_8x_ed_setup(&table, stdout, 128, 6, 32, TBL_RIGHT, 4, headers);
    char const *seq = sequence->str;
    struct imm_path const *path = &cli.pro.prod.path;

    char const *ocodon = cli.output.codon.seq;
    char const *oamino = cli.output.amino.seq;
    for (unsigned i = 0; i < imm_path_nsteps(path); ++i)
    {
        struct imm_step const *step = imm_path_step(path, i);

        bool is_match = protein_state_is_match(step->state_id);
        bool is_delete = protein_state_is_delete(step->state_id);
        char cons[2] = " ";
        if (is_match || is_delete)
            cons[0] =
                cli.pro.db.prof.consensus[protein_state_idx(step->state_id)];
        else if (protein_state_is_insert(step->state_id))
            cons[0] = '.';

        char codon[4] = {0};
        char amino[2] = {0};
        if (protein_state_is_mute(step->state_id))
        {
            codon[0] = ' ';
            codon[1] = ' ';
            codon[2] = ' ';
            amino[0] = ' ';
        }
        else
        {
            codon[0] = ocodon[0];
            codon[1] = ocodon[1];
            codon[2] = ocodon[2];
            amino[0] = *oamino;
            ocodon += 3;
            oamino++;
        }
        char str[6] = {0};
        memcpy(str, seq, step->seqlen);
        tbl_8x_ed_add_col(&table, TBL_LEFT,
                          (char const *[4]){cons, str, codon, amino});
        seq += step->seqlen;
    }

    tbl_8x_ed_flush(&table);
}

static enum rc predict_codons(struct imm_seq const *seq)
{
    struct protein_profile *prof = &cli.pro.db.prof;
    struct imm_path const *path = &cli.pro.prod.path;

    struct protein_codec codec = protein_codec_init(prof, path);
    struct imm_codon codon = imm_codon_any(prof->code->nuclt);

    enum rc rc = DONE;
    char *ocodon = cli.output.codon.seq;
    while (!(rc = protein_codec_next(&codec, seq, &codon)))
    {
        *(ocodon++) = imm_codon_asym(&codon);
        *(ocodon++) = imm_codon_bsym(&codon);
        *(ocodon++) = imm_codon_csym(&codon);
    }
    *ocodon = '\0';
    if (rc == END) rc = DONE;
    return rc;
}

static void decode_codons(void)
{
    char *ocodon = cli.output.codon.seq;
    char *oamino = cli.output.amino.seq;
    struct imm_codon codon = imm_codon_any(cli.pro.db.prof.code->nuclt);
    struct imm_abc const *abc = &codon.nuclt->super;
    while (*ocodon)
    {
        codon.a = imm_abc_symbol_idx(abc, *(ocodon++));
        codon.b = imm_abc_symbol_idx(abc, *(ocodon++));
        codon.c = imm_abc_symbol_idx(abc, *(ocodon++));
        *(oamino++) = imm_gc_decode(1, codon);
    }
    *oamino = '\0';
}

static enum rc write_codons(char const *ocodon)
{
    char const *id = cli.queries.fa.target.id;
    char const *desc = cli.queries.fa.target.desc;
    if (fasta_write(&cli.output.codon.fa, fasta_target(id, desc, ocodon), 60))
        return IOERROR;
    return DONE;
}

static enum rc write_aminos(char const *oamino)
{
    char const *id = cli.queries.fa.target.id;
    char const *desc = cli.queries.fa.target.desc;
    if (fasta_write(&cli.output.amino.fa, fasta_target(id, desc, oamino), 60))
        return IOERROR;
    return DONE;
}

static enum rc scan_queries(struct meta const *mt)
{
    struct imm_prod null = imm_prod();
    struct protein_profile *prof = &cli.pro.db.prof;
    struct imm_task *task = imm_task_new(&prof->alt.dp);
    if (!task) return error(FAIL, "failed to create task");

    enum fasta_rc fasta_rc = FASTA_SUCCESS;
    while (!(fasta_rc = fasta_read(&cli.queries.fa)))
    {
        struct imm_seq seq = seq_setup();
        if (imm_task_setup(task, &seq))
            return error(FAIL, "failed to create task");

        if (imm_dp_viterbi(&prof->alt.dp, task, &cli.pro.prod))
            return error(FAIL, "failed to run viterbi");

        if (imm_dp_viterbi(&prof->null.dp, task, &null))
            return error(FAIL, "failed to run viterbi");

        imm_float lrt = -2 * (null.loglik - cli.pro.prod.loglik);
        if (lrt < 100.0f) continue;

        enum rc rc = predict_codons(&seq);
        if (rc) return rc;

        char const *ocodon = cli.output.codon.seq;
        decode_codons();
        char const *oamino = cli.output.amino.seq;

        /* start = frag.interval.start; */
        /* stop = frag.interval.stop; */
        struct gff_feature *f = gff_set_feature(&cli.output.gff);
        gff_feature_set_seqid(f, cli.queries.fa.target.id);
        gff_feature_set_source(f, "deciphon");
        gff_feature_set_type(f, ".");
        gff_feature_set_start(f, ".");
        gff_feature_set_end(f, ".");
        gff_feature_set_score(f, "0.0");
        gff_feature_set_strand(f, "+");
        gff_feature_set_phase(f, ".");
        gff_feature_set_attrs(f, "wqwq");
        gff_write(&cli.output.gff);

        annotate(&seq, mt->name, cli.queries.fa.target.id);

        if ((rc = write_codons(ocodon))) return rc;
        if ((rc = write_aminos(oamino))) return rc;
        cli.output.nmatches++;
    }

    imm_del(task);
    imm_del(&null);
    return DONE;
}

static enum rc cli_setup(void)
{
    cli_log_setup();
    cli.queries.file = arguments.args[0];
    cli.pro.file = arguments.args[1];
    cli.pro.prod = imm_prod();
    cli.output.nmatches = 0;

    if (!(cli.queries.fd = fopen(cli.queries.file, "r")))
        return error(IOERROR, "failed to open queries file");

    progress_file_init(&cli.progress, cli.queries.fd);

    if (!(cli.pro.fd = fopen(cli.pro.file, "rb")))
        return error(IOERROR, "failed to open db file");

    cli.output.codon.file = default_ocodon;
    cli.output.amino.file = default_oamino;
    cli.output.file = default_output;

    if (!(cli.output.codon.fd = fopen(cli.output.codon.file, "w")))
        return error(IOERROR, "failed to open codon file");

    if (!(cli.output.amino.fd = fopen(cli.output.amino.file, "w")))
        return error(IOERROR, "failed to open amino file");

    if (!(cli.output.fd = fopen(cli.output.file, "w")))
        return error(IOERROR, "failed to open output file");

    fasta_init(&cli.output.codon.fa, cli.output.codon.fd, FASTA_WRITE);
    fasta_init(&cli.output.amino.fa, cli.output.amino.fd, FASTA_WRITE);

    cli.output.codon.seq[0] = '\0';
    cli.output.amino.seq[0] = '\0';
    gff_init(&cli.output.gff, cli.output.fd, GFF_WRITE);

    cli.pro.db = protein_db_default;

    enum rc rc = protein_db_openr(&cli.pro.db, cli.pro.fd);
    if (rc) return rc;

    return DONE;
}

static void queries_setup(void)
{
    fasta_init(&cli.queries.fa, cli.queries.fd, FASTA_READ);
    rewind(cli.queries.fd);
    cli.queries.fa.target.desc = cli.pro.db.prof.super.mt.acc;
}

enum rc cli_scan(int argc, char **argv)
{
    if (argp_parse(&argp, argc, argv, 0, 0, &arguments)) return ILLEGALARG;

    enum rc rc = cli_setup();
    if (rc) goto cleanup;

    assert(imm_abc_typeid(&cli.pro.db.nuclt.super) == IMM_DNA);
    gff_set_version(&cli.output.gff, NULL);
    gff_write(&cli.output.gff);

    progress_file_start(&cli.progress, !arguments.quiet);
    while (!(rc = dcp_db_end(&cli.pro.db.super)))
    {
        struct protein_profile *prof = protein_db_profile(&cli.pro.db);
        if ((rc = protein_db_read(&cli.pro.db, prof))) goto cleanup;

        queries_setup();
        struct meta const *mt = &cli.pro.db.prof.super.mt;
        if ((rc = scan_queries(mt))) goto cleanup;
        progress_file_update(&cli.progress);
    }
    if (rc != END) goto cleanup;

    rc = protein_db_close(&cli.pro.db);

cleanup:
    progress_file_stop(&cli.progress);
    imm_del(&cli.pro.prod);
    fclose(cli.queries.fd);
    fclose(cli.pro.fd);
    fclose(cli.output.codon.fd);
    fclose(cli.output.amino.fd);
    fclose(cli.output.fd);
    cli_log_flush();
    if (!rc)
    {
        if (cli.output.nmatches == 0)
        {
            printf("I'm sorry but no significant match has been found.\n");
        }
        else
        {
            printf(
                "I've found %d significant matches and saved them in <%s> and "
                "<%s>\n",
                cli.output.nmatches, cli.output.amino.file,
                cli.output.codon.file);
            printf("Annotation saved in <%s>.\n", cli.output.file);
        }
    }
    return rc;
}
