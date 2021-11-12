#include "work.h"
#include "db_handle.h"
#include "db_pool.h"
#include "dcp/generics.h"
#include "dcp/pro_state.h"
#include "dcp/version.h"
#include "error.h"
#include "macros.h"
#include "pro_match.h"
#include "prod_file.h"
#include "sched_db.h"
#include "sched_job.h"
#include "xfile.h"
#include "xstrlcpy.h"
#include <libgen.h>

enum dcp_rc open_work(struct work *work);
enum dcp_rc close_work(struct work *work);
enum dcp_rc next_profile(struct work *work);
enum dcp_rc prepare_task_for_dp(struct imm_task **task,
                                struct imm_dp const *dp);
enum dcp_rc prepare_task_for_prof(struct work *work, struct work_task *task);
enum dcp_rc prepare_task_for_seq(struct work_task *task, struct imm_seq *seq);
enum dcp_rc run_viterbi(struct work *work, struct work_task *task);
static inline imm_float compute_lrt(struct work_task const *task)
{
    return -2 * (task->null.prod.loglik - task->alt.prod.loglik);
}
static inline struct imm_abc const *get_abc(struct work const *work)
{
    return work->prof->super.abc;
}
static inline void prepare_prod(struct work_task *task)
{
    imm_prod_reset(&task->alt.prod);
    imm_prod_reset(&task->null.prod);
}
enum dcp_rc write_product(struct work *work, struct work_task *task,
                          unsigned match_id, struct imm_seq seq);
static inline enum dcp_rc work_task_fetch(struct work_task *task,
                                          int64_t seq_id)
{
    return sched_seq_get(&task->sched_seq, seq_id);
}

void work_init(struct work *work) { work->ntasks = 0; }

enum dcp_rc work_next(struct work *work)
{
    enum dcp_rc rc = DCP_DONE;
    int64_t job_id = 0;

    if ((rc = sched_job_next_pending(&job_id))) return rc;
    if ((rc = sched_job_get(&work->job, job_id))) return rc;

    work->db = db_pool_fetch(work->job.db_id);
    if (!work->db) return error(DCP_FAIL, "reached limit of open db handles");

    work->db_path[0] = 0;
    struct sched_db db = {0};
    if ((rc = sched_db_get(&db, work->job.db_id))) return rc;
    xstrlcpy(work->db_path, db.filepath, PATH_SIZE);

    int64_t seq_id = 0;
    work->ntasks = 0;
    while ((rc = sched_seq_next(job_id, &seq_id)) == DCP_NEXT)
    {
        struct work_task *task = &work->tasks[work->ntasks];
        if ((rc = work_task_fetch(task, seq_id))) goto cleanup;

        if (++work->ntasks >= ARRAY_SIZE(MEMBER_REF(*work, tasks)))
        {
            rc = error(DCP_FAIL, "too many tasks");
            goto cleanup;
        }
    }

cleanup:
    if (rc) return rc;
    return DCP_NEXT;
}

enum dcp_rc work_run(struct work *work)
{
    enum dcp_rc rc = open_work(work);
    if (rc) return rc;

    unsigned match_id = 1;
    while ((rc = next_profile(work)) == DCP_NEXT)
    {
        for (unsigned i = 0; i < work->ntasks; ++i)
        {
            struct work_task *task = &work->tasks[i];
            struct imm_seq seq =
                imm_seq(imm_str(task->sched_seq.data), get_abc(work));

            if ((rc = prepare_task_for_prof(work, task))) goto cleanup;
            if ((rc = prepare_task_for_seq(task, &seq))) goto cleanup;
            prepare_prod(task);
            if ((rc = run_viterbi(work, task))) goto cleanup;

            imm_float lrt = compute_lrt(&work->tasks[i]);
            if (lrt < 100.0f) continue;

            if ((rc = write_product(work, task, match_id, seq))) goto cleanup;
            match_id++;
        }
    }

    return close_work(work);

cleanup:
    close_work(work);
    return rc;
}

enum dcp_rc open_work(struct work *work)
{
    enum dcp_rc rc = prod_file_open(&work->prod_file);
    if (rc) goto cleanup;

    if ((rc = db_handle_open(work->db, work->db_path))) goto cleanup;
    return DCP_DONE;

cleanup:
    close_work(work);
    return rc;
}

enum dcp_rc close_work(struct work *work)
{
    enum dcp_rc rc = db_handle_close(work->db);
    if (rc) goto cleanup;
    rc = prod_file_close(&work->prod_file);
    if (rc) goto cleanup;
    FILE *fd = fopen(work->prod_file.path, "r");
    rc = sched_prod_add_from_tsv(fd);
    fclose(fd);
    if (rc) return rc;
    /* if (remove(file->path)) return error(DCP_IOERROR, "failed to remove
     * file"); */

cleanup:
    fclose(work->prod_file.fd);
    return rc;
}

enum dcp_rc next_profile(struct work *work)
{
    if (dcp_db_end(dcp_super(&work->db->pro))) return DCP_DONE;

    struct dcp_pro_prof *prof = dcp_pro_db_profile(&work->db->pro);

    enum dcp_rc rc = dcp_pro_db_read(&work->db->pro, prof);
    if (rc) return rc;

    work->prof = prof;

    return DCP_NEXT;
}

enum dcp_rc prepare_task_for_dp(struct imm_task **task, struct imm_dp const *dp)
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

enum dcp_rc prepare_task_for_prof(struct work *work, struct work_task *task)
{
    enum dcp_rc rc = prepare_task_for_dp(&task->alt.task, &work->prof->alt.dp);
    if (rc) return rc;
    return prepare_task_for_dp(&task->null.task, &work->prof->null.dp);
}

enum dcp_rc prepare_task_for_seq(struct work_task *task, struct imm_seq *seq)
{
    if (imm_task_setup(task->alt.task, seq))
        return error(DCP_FAIL, "failed to setup task");

    if (imm_task_setup(task->null.task, seq))
        return error(DCP_FAIL, "failed to setup task");

    return DCP_DONE;
}

enum dcp_rc run_viterbi(struct work *work, struct work_task *task)
{
    if (imm_dp_viterbi(&work->prof->alt.dp, task->alt.task, &task->alt.prod))
        return error(DCP_FAIL, "failed to run viterbi");

    if (imm_dp_viterbi(&work->prof->null.dp, task->null.task, &task->null.prod))
        return error(DCP_FAIL, "failed to run viterbi");

    return DCP_DONE;
}

enum dcp_rc write_product(struct work *work, struct work_task *task,
                          unsigned match_id, struct imm_seq seq)
{
    enum dcp_rc rc = DCP_DONE;
    struct imm_codon codon = imm_codon_any(work->prof->nuclt);

    struct dcp_meta const *mt = &work->prof->super.mt;

    sched_prod_set_job_id(&task->prod, work->job.id);
    sched_prod_set_seq_id(&task->prod, task->sched_seq.id);
    sched_prod_set_match_id(&task->prod, match_id);

    sched_prod_set_prof_name(&task->prod, mt->acc);
    sched_prod_set_abc_name(&task->prod, "dna_iupac");

    sched_prod_set_loglik(&task->prod, task->alt.prod.loglik);
    sched_prod_set_null_loglik(&task->prod, task->null.prod.loglik);

    sched_prod_set_model(&task->prod, "pro");
    sched_prod_set_version(&task->prod, DCP_VERSION);

    rc = sched_prod_write_preamble(&task->prod, work->prod_file.fd);
    if (rc) return rc;

    unsigned start = 0;
    struct imm_path const *path = &task->alt.prod.path;
    for (unsigned idx = 0; idx < imm_path_nsteps(path); idx++)
    {
        struct imm_step const *step = imm_path_step(path, idx);
        struct imm_seq frag = imm_subseq(&seq, start, step->seqlen);

        struct pro_match match = {0};
        pro_match_init(&match);
        pro_match_set_frag(&match, step->seqlen, frag.str);
        dcp_pro_prof_state_name(step->state_id,
                                pro_match_get_state_name(&match));

        if (!dcp_pro_state_is_mute(step->state_id))
        {
            rc = dcp_pro_prof_decode(work->prof, &frag, step->state_id, &codon);
            if (rc) return rc;
            pro_match_set_codon(&match, codon);
            pro_match_set_amino(&match, imm_gc_decode(1, codon));
        }
        if (idx > 0 && idx + 1 <= imm_path_nsteps(path))
        {
            rc = sched_prod_write_match_sep(work->prod_file.fd);
            if (rc) return rc;
        }
        if ((rc = sched_prod_write_match(work->prod_file.fd, &match)))
            return rc;
        start += step->seqlen;
    }
    rc = prod_file_write_nl(&work->prod_file);
    return rc;
}
