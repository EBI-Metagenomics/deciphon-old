#include "dcp/srv.h"
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
#include "pro_prod.h"
#include "sched.h"
#include "table.h"
#include "tbl/tbl.h"
#include "xstrlcpy.h"

struct dcp_srv
{
    struct sched sched;
    struct db_tbl db_tbl;
};

static enum dcp_rc prepare_db(struct dcp_srv *srv, dcp_sched_id db_id,
                              struct db **db);

struct dcp_srv *dcp_srv_open(char const *filepath)
{
    struct dcp_srv *srv = malloc(sizeof(*srv));
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

enum dcp_rc dcp_srv_close(struct dcp_srv *srv)
{
    enum dcp_rc rc = sched_close(&srv->sched);
    free(srv);
    return rc;
}

enum dcp_rc dcp_srv_add_db(struct dcp_srv *srv, char const *filepath,
                           dcp_sched_id *id)
{
    if (!file_readable(filepath))
        return error(DCP_IOERROR, "file is not readable");
    return sched_add_db(&srv->sched, filepath, id);
}

enum dcp_rc dcp_srv_submit_job(struct dcp_srv *srv, struct dcp_job *job,
                               dcp_sched_id db_id, dcp_sched_id *job_id)
{
    return sched_submit_job(&srv->sched, job, db_id, job_id);
}

enum dcp_rc dcp_srv_job_state(struct dcp_srv *srv, dcp_sched_id job_id,
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
    /* struct imm_path const *path = &cli.pro.prod.path; */

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
    /* struct imm_path const *path = &cli.pro.prod.path; */

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

struct prod_file
{
    struct file_tmp tmp;
    FILE *fd;
};

static enum dcp_rc prod_file_open(struct prod_file *file)
{
    file->tmp = FILE_TMP_INIT();
    enum dcp_rc rc = file_tmp_mk(&file->tmp);
    if (rc) return rc;
    if (!(file->fd = fopen(file->tmp.path, "wb")))
        rc = error(DCP_IOERROR, "failed to open prod file");
    return rc;
}

enum dcp_rc dcp_srv_run(struct dcp_srv *srv, bool blocking)
{
    struct dcp_job job;
    dcp_sched_id job_id = 0;
    dcp_sched_id db_id = 0;

    enum dcp_rc rc = sched_next_job(&srv->sched, &job, &job_id, &db_id);
    if (rc == DCP_DONE) return DCP_DONE;

    struct db *db = NULL;
    if ((rc = prepare_db(srv, db_id, &db))) return rc;

    struct prod_file prod_file = {0};
    if (!(rc = prod_file_open(&prod_file))) return rc;

    struct imm_prod alt = imm_prod();
    struct imm_prod null = imm_prod();
    unsigned match_id = 0;
    struct pro_prod prod = {0};
    while (!(rc = dcp_db_end(dcp_super(&db->pro))))
    {
        struct dcp_pro_prof *prof = dcp_pro_db_profile(&db->pro);
        if ((rc = dcp_pro_db_read(&db->pro, prof))) goto cleanup;
        struct imm_abc const *abc = prof->super.abc;
        struct imm_task *task = imm_task_new(&prof->alt.dp);
        if (!task) return error(DCP_FAIL, "failed to create task");

        dcp_sched_id seq_id = 0;
        struct dcp_seq seq = {0};
        char data[5001] = {0};
        while ((rc = sched_next_seq(&srv->sched, job_id, &seq_id, &seq) ==
                     DCP_NEXT))
        {
            struct imm_seq s = imm_seq(imm_str(data), abc);

            if (imm_task_setup(task, &s))
                return error(DCP_FAIL, "failed to create task");

            if (imm_dp_viterbi(&prof->alt.dp, task, &alt))
                return error(DCP_FAIL, "failed to run viterbi");

            if (imm_dp_viterbi(&prof->null.dp, task, &null))
                return error(DCP_FAIL, "failed to run viterbi");

            imm_float lrt = -2 * (null.loglik - alt.loglik);
            if (lrt < 100.0f) continue;

            struct pro_match match = {0};
            struct imm_step const *step = NULL;
            unsigned start = 0;
            for (unsigned idx = 0; idx < imm_path_nsteps(&alt.path); idx++)
            {
                step = imm_path_step(&alt.path, idx);
                /* if (!dcp_pro_state_is_mute(step->state_id)) break; */
                struct imm_seq frag = imm_subseq(&s, start, step->seqlen);
                xstrlcpy(match.frag, frag.str, frag.size + 1);

                dcp_pro_prof_state_name(step->state_id, match.state);

                struct imm_codon codon = imm_codon_any(prof->nuclt);
                rc = dcp_pro_prof_decode(prof, &frag, step->state_id, &codon);
                match.codon[0] = imm_codon_asym(&codon);
                match.codon[1] = imm_codon_bsym(&codon);
                match.codon[2] = imm_codon_csym(&codon);
                match.amino = imm_gc_decode(1, codon);
            }

            /* start = frag.interval.start; */
            /* stop = frag.interval.stop; */
            prod.super.match_id++;
            /* prod.super.seq_id = ""; */
            /* struct dcp_meta const *mt = &prof->super.mt; */

            /* annotate(&seq, mt->name, seq_id_str, &alt.path, ocodon,
             * oamino, prof); */

            match_id++;
        }
    }
    if (rc != DCP_END) goto cleanup;

    dcp_pro_db_close(&db->pro);

    return DCP_NEXT;

cleanup:
    dcp_pro_db_close(&db->pro);
    fclose(db->fd);
    db_tbl_del(&srv->db_tbl, &db->hnode);
    return rc;
}

static enum dcp_rc prepare_db(struct dcp_srv *srv, dcp_sched_id db_id,
                              struct db **db)
{
    *db = db_tbl_get(&srv->db_tbl, db_id);

    if (!*db && !(*db = db_tbl_new(&srv->db_tbl, db_id)))
        return error(DCP_FAIL, "reached limit of open dbs");

    char filepath[FILEPATH_SIZE] = {0};
    enum dcp_rc rc = DCP_DONE;
    if ((rc = sched_db_filepath(&srv->sched, db_id, filepath)))
    {
        db_tbl_del(&srv->db_tbl, &(*db)->hnode);
        return rc;
    }

    if (!((*db)->fd = fopen(filepath, "rb")))
    {
        db_tbl_del(&srv->db_tbl, &(*db)->hnode);
        return error(DCP_IOERROR, "failed to open db file");
    }

    if ((rc = dcp_pro_db_openr(&(*db)->pro, (*db)->fd)))
    {
        fclose((*db)->fd);
        db_tbl_del(&srv->db_tbl, &(*db)->hnode);
    }

    return rc;
}
