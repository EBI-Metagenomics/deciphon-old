#include "cli.h"
#include "dcp/cli.h"
#include "dcp/generics.h"
#include "dcp/pro_codec.h"
#include "dcp/pro_db.h"
#include "dcp/pro_reader.h"
#include "dcp/pro_state.h"
#include "error.h"
#include "fasta/fasta.h"
#include "gff/gff.h"
#include "progress_file.h"
#include "stream.h"
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

static char doc[] = "Scan targets -- dcp-scan targets.fna pfam.dcp";
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
    } targets;

    struct
    {
        char const *file;
        FILE *fd;
        struct dcp_pro_db db;
        struct imm_result result;
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

static struct imm_seq target_seq(void)
{
    char const *tgt = cli.targets.fa.target.seq;
    struct dcp_pro_prof *prof = &cli.pro.db.prof;
    struct imm_abc const *abc = &prof->nuclt->super;
    return imm_seq(imm_str(tgt), abc);
}

#if 0
def create_fragments(path: Path) -> Iterable[Tuple[Interval, Interval, bool]]:

    frag_start = frag_stop = 0
    step_start = step_stop = 0
    homologous = False

    for step_stop, step in enumerate(path):

        change = not homologous and step.state.name.startswith(b"M")
        change = change or homologous and step.state.name.startswith(b"E")
        change = change or not homologous and step.state.name.startswith(b"T")

        if change:
            if frag_start < frag_stop:
                fragi = Interval(frag_start, frag_stop)
                stepi = Interval(step_start, step_stop)
                yield (fragi, stepi, homologous)

            frag_start = frag_stop
            step_start = step_stop
            homologous = not homologous

        frag_stop += step.seq_len
#endif

static struct stream seq_row = STREAM_INIT(88);
static struct stream state_row = STREAM_INIT(88);

static void annotate(struct imm_seq const *sequence)
{
    unsigned const cell_size = 5;
    char const *seq = sequence->str;
    struct imm_path const *path = &cli.pro.result.path;
    char name[IMM_STATE_NAME_SIZE] = {0};

    if (imm_path_nsteps(path) == 0) return;

    unsigned i = 0;
    struct imm_step const *step = NULL;
    goto enter;
    for (; i < imm_path_nsteps(path); ++i)
    {
        if (stream_size(&state_row) == 0)
        {
            stream_flush(&seq_row, stdout);
            fprintf(stdout, "\n");
            stream_flush(&state_row, stdout);
            fprintf(stdout, "\n");
            fprintf(stdout, "\n");
        }
        stream_print(&state_row, " ", 1);
        stream_print(&seq_row, " ", 1);

    enter:
        step = imm_path_step(path, i);
        dcp_pro_state_name(step->state_id, name);

        if (stream_size(&state_row) < cell_size)
        {
            stream_flush(&seq_row, stdout);
            fprintf(stdout, "\n");
            stream_flush(&state_row, stdout);
            fprintf(stdout, "\n");
            fprintf(stdout, "\n");
        }

        unsigned n = (unsigned)strlen(name);
        stream_print(&state_row, name, n);
        for (unsigned k = 0; k < cell_size - n; ++k)
            stream_print(&state_row, " ", 1);

        stream_print(&seq_row, seq, step->seqlen);
        seq += step->seqlen;
        for (unsigned k = 0; k < cell_size - step->seqlen; ++k)
            stream_print(&seq_row, " ", 1);
    }

    stream_flush(&seq_row, stdout);
    fprintf(stdout, "\n");
    stream_flush(&state_row, stdout);
    fprintf(stdout, "\n");
    fprintf(stdout, "\n");
}

static enum dcp_rc predict_codons(struct imm_seq const *seq)
{
    struct dcp_pro_prof *prof = &cli.pro.db.prof;
    struct imm_path const *path = &cli.pro.result.path;

    struct dcp_pro_codec codec = dcp_pro_codec_init(prof, path);
    struct imm_codon codon = imm_codon_any(prof->nuclt);

    enum dcp_rc rc = DCP_SUCCESS;
    char *ocodon = cli.output.codon.seq;
    while (!(rc = dcp_pro_codec_next(&codec, seq, &codon)))
    {
        *(ocodon++) = imm_codon_asym(&codon);
        *(ocodon++) = imm_codon_bsym(&codon);
        *(ocodon++) = imm_codon_csym(&codon);
    }
    *ocodon = '\0';
    if (rc == DCP_END) rc = DCP_SUCCESS;
    return rc;
}

static void decode_codons(void)
{
    char *ocodon = cli.output.codon.seq;
    char *oamino = cli.output.amino.seq;
    struct imm_codon codon = imm_codon_any(cli.pro.db.prof.nuclt);
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

static enum dcp_rc write_codons(char const *ocodon)
{
    char const *id = cli.targets.fa.target.id;
    char const *desc = cli.targets.fa.target.desc;
    if (fasta_write(&cli.output.codon.fa, fasta_target(id, desc, ocodon), 60))
        return DCP_IOERROR;
    return DCP_SUCCESS;
}

static enum dcp_rc write_aminos(char const *oamino)
{
    char const *id = cli.targets.fa.target.id;
    char const *desc = cli.targets.fa.target.desc;
    if (fasta_write(&cli.output.amino.fa, fasta_target(id, desc, oamino), 60))
        return DCP_IOERROR;
    return DCP_SUCCESS;
}

static enum dcp_rc targets_scan(struct dcp_meta const *mt)
{
    struct imm_result null = imm_result();
    struct dcp_pro_prof *prof = &cli.pro.db.prof;
    struct imm_task *task = imm_task_new(&prof->alt.dp);
    if (!task) return error(DCP_RUNTIMEERROR, "failed to create task");

    enum fasta_rc fasta_rc = FASTA_SUCCESS;
    while (!(fasta_rc = fasta_read(&cli.targets.fa)))
    {
        struct imm_seq seq = target_seq();
        if (imm_task_setup(task, &seq))
            return error(DCP_RUNTIMEERROR, "failed to create task");

        if (imm_dp_viterbi(&prof->alt.dp, task, &cli.pro.result))
            return error(DCP_RUNTIMEERROR, "failed to run viterbi");

        if (imm_dp_viterbi(&prof->null.dp, task, &null))
            return error(DCP_RUNTIMEERROR, "failed to run viterbi");

        imm_float lrt = -2 * (null.loglik - cli.pro.result.loglik);
        if (lrt < 100.0f) continue;

        enum dcp_rc rc = predict_codons(&seq);
        if (rc) return rc;

        char const *ocodon = cli.output.codon.seq;
        decode_codons();
        char const *oamino = cli.output.amino.seq;

        /* start = frag.interval.start; */
        /* stop = frag.interval.stop; */
        struct gff_feature *f = gff_set_feature(&cli.output.gff);
        gff_feature_set_seqid(f, cli.targets.fa.target.id);
        gff_feature_set_source(f, "deciphon");
        gff_feature_set_type(f, ".");
        gff_feature_set_start(f, ".");
        gff_feature_set_end(f, ".");
        gff_feature_set_score(f, "0.0");
        gff_feature_set_strand(f, "+");
        gff_feature_set_phase(f, ".");
        gff_feature_set_attrs(f, "wqwq");
        gff_write(&cli.output.gff);

        printf("Name: %s\n", mt->name);
        printf("ACC: %s\n", mt->acc);
        annotate(&seq);

        if ((rc = write_codons(ocodon))) return rc;
        if ((rc = write_aminos(oamino))) return rc;
        cli.output.nmatches++;
    }

    imm_del(task);
    imm_del(&null);
    return DCP_SUCCESS;
}

static enum dcp_rc cli_setup(void)
{
    cli_log_setup();
    cli.targets.file = arguments.args[0];
    cli.pro.file = arguments.args[1];
    cli.pro.result = imm_result();
    cli.output.nmatches = 0;

    if (!(cli.targets.fd = fopen(cli.targets.file, "r")))
        return error(DCP_IOERROR, "failed to open targets file");

    progress_file_init(&cli.progress, cli.targets.fd);

    if (!(cli.pro.fd = fopen(cli.pro.file, "rb")))
        return error(DCP_IOERROR, "failed to open db file");

    cli.output.codon.file = default_ocodon;
    cli.output.amino.file = default_oamino;
    cli.output.file = default_output;

    if (!(cli.output.codon.fd = fopen(cli.output.codon.file, "w")))
        return error(DCP_IOERROR, "failed to open codon file");

    if (!(cli.output.amino.fd = fopen(cli.output.amino.file, "w")))
        return error(DCP_IOERROR, "failed to open amino file");

    if (!(cli.output.fd = fopen(cli.output.file, "w")))
        return error(DCP_IOERROR, "failed to open output file");

    fasta_init(&cli.output.codon.fa, cli.output.codon.fd, FASTA_WRITE);
    fasta_init(&cli.output.amino.fa, cli.output.amino.fd, FASTA_WRITE);

    cli.output.codon.seq[0] = '\0';
    cli.output.amino.seq[0] = '\0';
    gff_init(&cli.output.gff, cli.output.fd, GFF_WRITE);

    cli.pro.db = dcp_pro_db_default;

    enum dcp_rc rc = dcp_pro_db_openr(&cli.pro.db, cli.pro.fd);
    if (rc) return rc;

    return DCP_SUCCESS;
}

static void targets_setup(void)
{
    fasta_init(&cli.targets.fa, cli.targets.fd, FASTA_READ);
    rewind(cli.targets.fd);
    cli.targets.fa.target.desc = cli.pro.db.prof.super.mt.acc;
}

enum dcp_rc dcp_cli_scan(int argc, char **argv)
{
    if (argp_parse(&argp, argc, argv, 0, 0, &arguments)) return DCP_ILLEGALARG;

    enum dcp_rc rc = cli_setup();
    if (rc) goto cleanup;

    assert(imm_abc_typeid(&cli.pro.db.nuclt.super) == IMM_DNA);
    gff_set_version(&cli.output.gff, NULL);
    gff_write(&cli.output.gff);

    progress_file_start(&cli.progress, !arguments.quiet);
    while (!(rc = dcp_db_end(dcp_super(&cli.pro.db))))
    {
        struct dcp_pro_prof *prof = dcp_pro_db_profile(&cli.pro.db);
        if ((rc = dcp_pro_db_read(&cli.pro.db, prof))) goto cleanup;

        targets_setup();
        struct dcp_meta const *mt = &cli.pro.db.prof.super.mt;
        if ((rc = targets_scan(mt))) goto cleanup;
        progress_file_update(&cli.progress);
    }
    if (rc != DCP_END) goto cleanup;

    rc = dcp_pro_db_close(&cli.pro.db);

cleanup:
    progress_file_stop(&cli.progress);
    imm_del(&cli.pro.result);
    fclose(cli.targets.fd);
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
