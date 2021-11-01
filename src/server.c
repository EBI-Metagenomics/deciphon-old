#include "dcp/server.h"
#include "cco/cco.h"
#include "db_tbl.h"
#include "dcp/db.h"
#include "dcp/generics.h"
#include "dcp/job.h"
#include "dcp/pro_codec.h"
#include "dcp/pro_db.h"
#include "dcp/pro_state.h"
#include "dcp/prof_types.h"
#include "dcp_file.h"
#include "error.h"
#include "fasta/fasta.h"
#include "filepath.h"
#include "gff/gff.h"
#include "sched.h"
#include "table.h"
#include "tbl/tbl.h"

struct dcp_server
{
    struct sched sched;
    struct db_tbl db_tbl;
};

struct dcp_server *dcp_server_open(char const *filepath)
{
    struct dcp_server *srv = malloc(sizeof(*srv));
    if (!srv)
    {
        error(DCP_OUTOFMEM, "failed to malloc server");
        goto cleanup;
    }
    db_tbl_init(&srv->db_tbl);

    enum dcp_rc rc = DCP_DONE;
    if ((rc = sched_setup(filepath))) goto cleanup;
    if ((rc = sched_open(&srv->sched, filepath))) goto cleanup;

    return srv;

cleanup:
    free(srv);
    return NULL;
}

enum dcp_rc dcp_server_close(struct dcp_server *srv)
{
    enum dcp_rc rc = sched_close(&srv->sched);
    free(srv);
    return rc;
}

enum dcp_rc dcp_server_add_db(struct dcp_server *srv, char const *filepath,
                              dcp_sched_id *id)
{
    if (!file_readable(filepath))
        return error(DCP_IOERROR, "file is not readable");
    return sched_add_db(&srv->sched, filepath, id);
}

enum dcp_rc dcp_server_submit_job(struct dcp_server *srv, struct dcp_job *job,
                                  dcp_sched_id db_id, dcp_sched_id *job_id)
{
    return sched_submit_job(&srv->sched, job, db_id, job_id);
}

enum dcp_rc dcp_server_job_state(struct dcp_server *srv, dcp_sched_id job_id,
                                 enum dcp_job_state *state)
{
    return sched_job_state(&srv->sched, job_id, state);
}

static enum dcp_rc predict_codons(struct imm_seq const *seq,
                                  struct dcp_pro_prof *prof,
                                  struct imm_path const *path,
                                  char ocodon[5001 * 3])
{
    /* struct dcp_pro_prof *prof = &cli.pro.db.prof; */
    /* struct imm_path const *path = &cli.pro.result.path; */

    struct dcp_pro_codec codec = dcp_pro_codec_init(prof, path);
    struct imm_codon codon = imm_codon_any(prof->nuclt);

    enum dcp_rc rc = DCP_DONE;
    /* char *ocodon = cli.output.codon.seq; */
    while (!(rc = dcp_pro_codec_next(&codec, seq, &codon)))
    {
        *(ocodon++) = imm_codon_asym(&codon);
        *(ocodon++) = imm_codon_bsym(&codon);
        *(ocodon++) = imm_codon_csym(&codon);
    }
    *ocodon = '\0';
    if (rc == DCP_END) rc = DCP_DONE;
    return rc;
}

static void decode_codons(char ocodon[5001 * 3], char oamino[5001],
                          struct imm_nuclt const *nuclt)
{
    struct imm_codon codon = imm_codon_any(nuclt);
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

static void annotate(struct imm_seq const *sequence, char const *profile_name,
                     char const *seq_name, struct imm_path const *path,
                     char const ocodon[5001 * 3], char const oamino[5001 * 3],
                     struct dcp_pro_prof *prof)
{
    static struct tbl_8x_ed table = {0};
    char const *headers[4] = {profile_name, seq_name, "posterior", ""};
    tbl_8x_ed_setup(&table, stdout, 128, 6, 32, TBL_RIGHT, 4, headers);
    char const *seq = sequence->str;
    /* struct imm_path const *path = &cli.pro.result.path; */

    /* char const *ocodon = cli.output.codon.seq; */
    /* char const *oamino = cli.output.amino.seq; */
    for (unsigned i = 0; i < imm_path_nsteps(path); ++i)
    {
        struct imm_step const *step = imm_path_step(path, i);

        bool is_match = dcp_pro_state_is_match(step->state_id);
        bool is_delete = dcp_pro_state_is_delete(step->state_id);
        char cons[2] = " ";
        /* cli.pro.db.prof.consensus[dcp_pro_state_idx(step->state_id)]; */
        if (is_match || is_delete)
            cons[0] = prof->consensus[dcp_pro_state_idx(step->state_id)];
        else if (dcp_pro_state_is_insert(step->state_id))
            cons[0] = '.';

        char codon[4] = {0};
        char amino[2] = {0};
        if (dcp_pro_state_is_mute(step->state_id))
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

enum dcp_rc dcp_server_run(struct dcp_server *srv, bool blocking)
{
    struct dcp_job job;
    dcp_sched_id job_id = 0;
    dcp_sched_id db_id = 0;
    enum dcp_rc rc = sched_next_job(&srv->sched, &job, &job_id, &db_id);
    if (rc == DCP_DONE) return DCP_DONE;

    struct db *db = db_tbl_get(&srv->db_tbl, db_id);
    if (!db)
    {
        if (!(db = db_tbl_new(&srv->db_tbl, db_id)))
        {
            rc = error(DCP_FAIL, "reached limit of open dbs");
            goto cleanup;
        }
    }

    char filepath[FILEPATH_SIZE] = {0};
    if ((rc = sched_db_filepath(&srv->sched, db_id, filepath))) goto cleanup;

    if (!(db->fd = fopen(filepath, "rb")))
        return error(DCP_IOERROR, "failed to open db file");

    if ((rc = dcp_pro_db_openr(&db->pro, db->fd))) goto cleanup;

    FILE *gff_fd = fopen("output.gff", "w");
    struct gff gff;
    gff_init(&gff, gff_fd, GFF_WRITE);
    gff_set_version(&gff, NULL);
    gff_write(&gff);

    struct imm_result result = imm_result();
    struct imm_result null = imm_result();
    struct fasta codon_fa;
    struct fasta amino_fa;
    FILE *codon_fd = fopen("codon.fna", "w");
    FILE *amino_fd = fopen("amino.faa", "w");
    fasta_init(&codon_fa, codon_fd, FASTA_WRITE);
    fasta_init(&amino_fa, amino_fd, FASTA_WRITE);
    int nmatches = 0;
    while (!(rc = dcp_db_end(dcp_super(&db->pro))))
    {
        struct dcp_pro_prof *prof = dcp_pro_db_profile(&db->pro);
        if ((rc = dcp_pro_db_read(&db->pro, prof))) goto cleanup;
        struct imm_abc const *abc = prof->super.abc;
        struct imm_task *task = imm_task_new(&prof->alt.dp);
        if (!task) return error(DCP_FAIL, "failed to create task");

        dcp_sched_id seq_id = 0;
        char data[5001] = {0};
        while ((rc = sched_next_seq(&srv->sched, job_id, &seq_id, data) ==
                     DCP_NEXT))
        {
            struct imm_seq seq = imm_seq(imm_str(data), abc);
            /* printf("DATA: %s\n", data); */

            if (imm_task_setup(task, &seq))
                return error(DCP_FAIL, "failed to create task");

            if (imm_dp_viterbi(&prof->alt.dp, task, &result))
                return error(DCP_FAIL, "failed to run viterbi");

            if (imm_dp_viterbi(&prof->null.dp, task, &null))
                return error(DCP_FAIL, "failed to run viterbi");

            imm_float lrt = -2 * (null.loglik - result.loglik);
            if (lrt < 100.0f) continue;

            /* struct dcp_pro_prof *prof = &cli.pro.db.prof; */
            /* struct imm_path const *path = &cli.pro.result.path; */
            char ocodon[5001 * 3] = {0};
            char oamino[5001 * 3] = {0};
            enum dcp_rc rc0 = predict_codons(&seq, prof, &result.path, ocodon);
            if (rc0) return rc0;

            decode_codons(ocodon, oamino, prof->nuclt);

            /* start = frag.interval.start; */
            /* stop = frag.interval.stop; */
            struct dcp_meta const *mt = &prof->super.mt;
            struct gff_feature *f = gff_set_feature(&gff);
            char seq_id_str[8] = {0};
            sprintf(seq_id_str, "%lld.%d", seq_id, nmatches);
            gff_feature_set_seqid(f, seq_id_str);
            gff_feature_set_source(f, "deciphon");
            gff_feature_set_type(f, ".");
            gff_feature_set_start(f, ".");
            gff_feature_set_end(f, ".");
            gff_feature_set_score(f, "0.0");
            gff_feature_set_strand(f, "+");
            gff_feature_set_phase(f, ".");
            char attrs[100] = {0};
            sprintf(attrs, "ID=%s;Name=%s", mt->acc, mt->name);
            gff_feature_set_attrs(f, attrs);
            gff_write(&gff);

            /* annotate(&seq, mt->name, seq_id_str, &result.path, ocodon,
             * oamino, prof); */

            if (fasta_write(&codon_fa, fasta_target(seq_id_str, "", ocodon),
                            60))
                return DCP_IOERROR;

            if (fasta_write(&amino_fa, fasta_target(seq_id_str, "", oamino),
                            60))
                return DCP_IOERROR;

            nmatches++;
        }
    }
    if (rc != DCP_END) goto cleanup;

    fclose(codon_fd);
    fclose(amino_fd);
    fclose(gff_fd);
    dcp_pro_db_close(&db->pro);

    /* sched_add_result(&srv->sched, job_id, "output.gff", "codon.fna", "amino.faa"); */

    return DCP_NEXT;

cleanup:
    dcp_pro_db_close(&db->pro);
    fclose(db->fd);
    db_tbl_del(&srv->db_tbl, &db->hnode);
    return rc;
}
