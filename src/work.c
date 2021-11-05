#include "work.h"
#include <libgen.h>
#include "db_pool.h"
#include "dcp.h"
#include "dcp/generics.h"
#include "dcp/pro_state.h"
#include "dcp/version.h"
#include "error.h"
#include "pro_match.h"
#include "prod.h"
#include "sched.h"
#include "xfile.h"
#include "xstrlcpy.h"

static enum dcp_rc open_work(struct work *work);
static enum dcp_rc close_work(struct work *work);
static enum dcp_rc next_profile(struct work *work);
static enum dcp_rc next_seq(struct work *work);
static inline void rewind_seqs(struct work *work) { work->seq.id = 0; }
static enum dcp_rc prepare_task_for_prof(struct work *work);
static enum dcp_rc prepare_task_for_seq(struct work *work);
static void prepare_prod(struct work *work);
static enum dcp_rc run_viterbi(struct work *work);
static inline imm_float compute_lrt(struct work const *work)
{
    return -2 * (work->prod.null.loglik - work->prod.alt.loglik);
}

void work_init(struct work *work, struct sched *sched)
{
    work->sched = sched;
    work->task.alt = NULL;
    work->task.null = NULL;
    work->prod.alt = imm_prod();
    work->prod.null = imm_prod();
}

enum dcp_rc work_fetch(struct work *work, struct db_pool *pool)
{
    enum dcp_rc rc = sched_next_job(work->sched, &work->job);
    if (rc == DCP_DONE) return DCP_DONE;
    work->db = db_pool_fetch(pool, work->job.db_id);
    if (!work->db) return error(DCP_FAIL, "reached limit of open db handles");

    work->db_path[0] = 0;
    if ((rc = sched_db_filepath(work->sched, work->job.db_id, work->db_path)))
        return rc;
    return DCP_NEXT;
}

static enum dcp_rc write_product(struct work *work, unsigned match_id)
{
    enum dcp_rc rc = DCP_DONE;
    struct pro_match match = {0};
    unsigned start = 0;
    char state[IMM_STATE_NAME_SIZE] = {0};
    struct imm_codon codon = imm_codon_any(work->prof->nuclt);

    struct dcp_meta const *mt = &work->prof->super.mt;
    char db_path[PATH_SIZE]={0};
    xstrlcpy(db_path, work->db_path, PATH_SIZE);
    prod_setup(&work->prod_dcp, match_id, work->seq.dcp.id, mt->acc, 0, 0,
               "dna_iupac", work->prod.alt.loglik, work->prod.null.loglik,
               "pro", DCP_VERSION, basename(db_path), "xxh3:xxxxxxxxxx");
    rc = prod_write(&work->prod_dcp, work->prod_file.fd);
    if (rc) return rc;
    rc = prod_file_write_sep(&work->prod_file);
    if (rc) return rc;

    struct imm_seq const *seq = &work->seq.imm;
    struct imm_path const *path = &work->prod.alt.path;
    for (unsigned idx = 0; idx < imm_path_nsteps(path); idx++)
    {
        struct imm_step const *step = imm_path_step(path, idx);
        struct imm_seq frag = imm_subseq(seq, start, step->seqlen);
        dcp_pro_prof_state_name(step->state_id, state);

        pro_match_setup(&match, imm_subseq(seq, start, step->seqlen), state);
        if (!dcp_pro_state_is_mute(step->state_id))
        {
            rc = dcp_pro_prof_decode(work->prof, &frag, step->state_id, &codon);
            if (rc) return rc;
            pro_match_set_codon(&match, codon);
            pro_match_set_amino(&match, imm_gc_decode(1, codon));
        }
        if (idx > 0) {
            rc = pro_match_write_sep(work->prod_file.fd);
            if (rc) return rc;
        }
        if ((rc = pro_match_write(&match, work->prod_file.fd))) return rc;
        start += step->seqlen;
    }
    rc = prod_file_write_nl(&work->prod_file);
    return rc;
}

enum dcp_rc work_run(struct work *work)
{
    enum dcp_rc rc = open_work(work);
    if (rc) return rc;

    unsigned match_id = 1;
    while ((rc = next_profile(work)) == DCP_NEXT)
    {
        rewind_seqs(work);
        if ((rc = prepare_task_for_prof(work))) goto cleanup;

        while ((rc = next_seq(work)) == DCP_NEXT)
        {
            if ((rc = prepare_task_for_seq(work))) goto cleanup;
            prepare_prod(work);
            if ((rc = run_viterbi(work))) goto cleanup;

            imm_float lrt = compute_lrt(work);
            if (lrt < 100.0f) continue;

            if ((rc = write_product(work, match_id))) goto cleanup;
            match_id++;
        }
    }

    return close_work(work);

cleanup:
    close_work(work);
    return rc;
}

static enum dcp_rc open_work(struct work *work)
{
    work->db->fd = NULL;
    enum dcp_rc rc = prod_file_open(&work->prod_file);
    if (rc) goto cleanup;

    work->db->fd = fopen(work->db_path, "rb");
    if (!work->db->fd)
    {
        rc = error(DCP_IOERROR, "failed to open db");
        goto cleanup;
    }

    if ((rc = dcp_pro_db_openr(&work->db->pro, work->db->fd))) goto cleanup;

    return DCP_DONE;

cleanup:
    close_work(work);
    return rc;
}

static enum dcp_rc close_work(struct work *work)
{
    enum dcp_rc rc = dcp_pro_db_close(&work->db->pro);
    if (rc) return rc;
    if (work->db->fd && fclose(work->db->fd))
        return error(DCP_IOERROR, "failed to close file");
    return prod_file_close(&work->prod_file);
}

static enum dcp_rc next_profile(struct work *work)
{
    if (dcp_db_end(dcp_super(&work->db->pro))) return DCP_DONE;

    struct dcp_pro_prof *prof = dcp_pro_db_profile(&work->db->pro);

    enum dcp_rc rc = dcp_pro_db_read(&work->db->pro, prof);
    if (rc) return rc;

    work->prof = prof;
    work->abc = work->prof->super.abc;

    return DCP_NEXT;
}

static enum dcp_rc next_seq(struct work *work)
{
    enum dcp_rc rc = sched_next_seq(work->sched, work->job.id, &work->seq.id,
                                    &work->seq.dcp);
    if (rc == DCP_DONE) return rc;
    work->seq.imm = imm_seq(imm_str(work->seq.dcp.data), work->abc);
    return rc;
}

static enum dcp_rc prepare_task_for_dp(struct imm_task **task,
                                       struct imm_dp const *dp)
{
    if (*task)
    {
        if (imm_task_reset(*task, dp))
            return error(DCP_FAIL, "failed to reset task");
    }
    else
    {
        if (!(*task = imm_task_new(dp)))
            return error(DCP_FAIL, "failed to create task");
    }
    return DCP_DONE;
}

static enum dcp_rc prepare_task_for_prof(struct work *work)
{
    enum dcp_rc rc = prepare_task_for_dp(&work->task.alt, &work->prof->alt.dp);
    if (rc) return rc;
    return prepare_task_for_dp(&work->task.null, &work->prof->null.dp);
}

static enum dcp_rc prepare_task_for_seq(struct work *work)
{
    if (imm_task_setup(work->task.alt, &work->seq.imm))
        return error(DCP_FAIL, "failed to setup task");

    if (imm_task_setup(work->task.null, &work->seq.imm))
        return error(DCP_FAIL, "failed to setup task");

    return DCP_DONE;
}

static void prepare_prod(struct work *work)
{
    imm_prod_reset(&work->prod.alt);
    imm_prod_reset(&work->prod.null);
}

static enum dcp_rc run_viterbi(struct work *work)
{
    if (imm_dp_viterbi(&work->prof->alt.dp, work->task.alt, &work->prod.alt))
        return error(DCP_FAIL, "failed to run viterbi");

    if (imm_dp_viterbi(&work->prof->null.dp, work->task.null, &work->prod.null))
        return error(DCP_FAIL, "failed to run viterbi");

    return DCP_DONE;
}
