#include "work.h"
#include "db_handle.h"
#include "db_pool.h"
#include "logger.h"
#include "macros.h"
#include "protein_match.h"
#include "protein_state.h"
#include "safe.h"
#include "sched_db.h"
#include "sched_job.h"
#include "tok.h"
#include "utc.h"
#include "version.h"
#include "xmath.h"
#include <libgen.h>

enum rc open_work(struct work *work, unsigned num_threads);
enum rc close_work(struct work *work);
enum rc next_profile(struct work *work);
enum rc prepare_task_for_dp(struct imm_task **task, struct imm_dp const *dp);
enum rc prepare_task_for_prof(struct work *work, struct task *task);
enum rc prepare_task_for_seq(struct task *task, struct imm_seq *seq);
enum rc run_viterbi(struct work *work, struct task *task);
static inline double compute_lrt(struct task const *task)
{
    printf("Nul (%f) Alt (%f)\n", task->null.prod.loglik,
           task->alt.prod.loglik);
    return xmath_lrt(task->null.prod.loglik, task->alt.prod.loglik);
}
static inline struct imm_abc const *get_abc(struct work const *work)
{
    return work->prof->super.code->abc;
}
static inline void prepare_prod(struct task *task)
{
    imm_prod_reset(&task->alt.prod);
    imm_prod_reset(&task->null.prod);
}
enum rc write_product(struct work *work, struct task *task, unsigned match_id,
                      struct imm_seq seq);

void work_init(struct work *work)
{
    work->ntasks = 0;
    atomic_store(&work->failed, false);
    /* TODO: review it */
    work->tok = tok_new(64000);
}

enum rc work_next(struct work *work)
{
    enum rc rc = DONE;
    int64_t job_id = 0;

    if ((rc = sched_job_next_pending(&job_id))) return rc;
    if ((rc = sched_job_get(&work->job, job_id))) return rc;

    work->db = db_pool_fetch(work->job.db_id);
    if (!work->db) return error(FAIL, "reached limit of open db handles");

    work->db_path[0] = 0;
    struct sched_db db = {0};
    if ((rc = sched_db_get_by_id(&db, work->job.db_id))) return rc;
    safe_strcpy(work->db_path, db.filepath, DCP_PATH_SIZE);

    int64_t seq_id = 0;
    work->ntasks = 0;
    while ((rc = sched_seq_next(job_id, &seq_id)) == NEXT)
    {
        struct task *task = &work->tasks[work->ntasks];
        if ((rc = task_setup(task, get_abc(work), seq_id))) goto cleanup;

        if (++work->ntasks >= ARRAY_SIZE(MEMBER_REF(*work, tasks)))
        {
            rc = error(FAIL, "too many tasks");
            goto cleanup;
        }
    }

cleanup:
    if (rc) return rc;
    return NEXT;
}

enum rc work_run(struct work *work, unsigned num_threads)
{
    enum rc rc = open_work(work, num_threads);
    if (rc) return rc;

    unsigned match_id = 1;
    while ((rc = next_profile(work)) == NEXT)
    {
        for (unsigned i = 0; i < work->ntasks; ++i)
        {
            struct task *task = &work->tasks[i];
            struct imm_seq seq = task->seq;

            if ((rc = prepare_task_for_prof(work, task))) goto cleanup;
            if ((rc = prepare_task_for_seq(task, &seq))) goto cleanup;
            prepare_prod(task);
            if ((rc = run_viterbi(work, task))) goto cleanup;

            double lrt = compute_lrt(&work->tasks[i]);
            printf("Match_id (%d), lrt (%f)\n", match_id, lrt);
            if (lrt < -100000.f) printf("Weird lrt\n");
            if (lrt < 100.0f) continue;

            if ((rc = write_product(work, task, match_id, seq))) goto cleanup;
            match_id++;
        }
    }
    printf("\n");

    return close_work(work);

cleanup:
    atomic_store(&work->failed, true);
    close_work(work);
    return rc;
}

enum rc open_work(struct work *work, unsigned num_threads)
{
    enum rc rc = xfile_tmp_open(&work->prod_file);
    if (rc) goto cleanup;

    rc = db_handle_open(work->db, work->db_path, num_threads);
    if (rc) goto cleanup;
    return DONE;

cleanup:
    close_work(work);
    return rc;
}

enum rc close_work(struct work *work)
{
    enum rc rc = db_handle_close(work->db);
    if (rc) goto cleanup;
    if ((rc = xfile_tmp_rewind(&work->prod_file))) goto cleanup;

    int64_t exec_ended = (int64_t)utc_now();
    if (work->failed)
    {
        rc = sched_job_set_error(work->job.id, "some error", exec_ended);
        if (rc) goto cleanup;
    }
    else
    {
        rc = sched_prod_add_from_tsv(work->prod_file.fp, work->tok);
        if (rc) goto cleanup;
        rc = sched_job_set_done(work->job.id, exec_ended);
        if (rc) goto cleanup;
    }

cleanup:
    xfile_tmp_destroy(&work->prod_file);
    tok_del(work->tok);
    return rc;
}

enum rc next_profile(struct work *work)
{
    if (dcp_db_end(&work->db->pro.super)) return DONE;

    struct protein_profile *prof = protein_db_profile(&work->db->pro);

    enum rc rc = protein_db_read(&work->db->pro, prof);
    if (rc) return rc;

    work->prof = prof;

    return NEXT;
}

enum rc prepare_task_for_dp(struct imm_task **task, struct imm_dp const *dp)
{
    if (*task)
    {
        if (imm_task_reset(*task, dp))
            return error(FAIL, "failed to reset task");
    }
    else
    {
        if (!(*task = imm_task_new(dp)))
            return error(FAIL, "failed to create task");
    }
    return DONE;
}

enum rc prepare_task_for_prof(struct work *work, struct task *task)
{
    enum rc rc = prepare_task_for_dp(&task->alt.task, &work->prof->alt.dp);
    if (rc) return rc;
    return prepare_task_for_dp(&task->null.task, &work->prof->null.dp);
}

enum rc prepare_task_for_seq(struct task *task, struct imm_seq *seq)
{
    if (imm_task_setup(task->alt.task, seq))
        return error(FAIL, "failed to setup task");

    if (imm_task_setup(task->null.task, seq))
        return error(FAIL, "failed to setup task");

    return DONE;
}

enum rc run_viterbi(struct work *work, struct task *task)
{
    if (imm_dp_viterbi(&work->prof->alt.dp, task->alt.task, &task->alt.prod))
        return error(FAIL, "failed to run viterbi");

    if (imm_dp_viterbi(&work->prof->null.dp, task->null.task, &task->null.prod))
        return error(FAIL, "failed to run viterbi");

    return DONE;
}

enum rc write_product(struct work *work, struct task *task, unsigned match_id,
                      struct imm_seq seq)
{
    enum rc rc = DONE;
    struct imm_codon codon = imm_codon_any(work->prof->code->nuclt);

    struct meta const *mt = &work->prof->super.mt;

    sched_prod_set_job_id(&task->prod, work->job.id);
    sched_prod_set_seq_id(&task->prod, task->sched_seq.id);
    sched_prod_set_match_id(&task->prod, match_id);

    sched_prod_set_prof_name(&task->prod, mt->acc);
    sched_prod_set_abc_name(&task->prod, "dna_iupac");

    sched_prod_set_loglik(&task->prod, task->alt.prod.loglik);
    sched_prod_set_null_loglik(&task->prod, task->null.prod.loglik);

    sched_prod_set_model(&task->prod, "pro");
    sched_prod_set_version(&task->prod, VERSION);

    rc = sched_prod_write_preamble(&task->prod, work->prod_file.fp);
    if (rc) return rc;

    unsigned start = 0;
    struct imm_path const *path = &task->alt.prod.path;
    for (unsigned idx = 0; idx < imm_path_nsteps(path); idx++)
    {
        struct imm_step const *step = imm_path_step(path, idx);
        struct imm_seq frag = imm_subseq(&seq, start, step->seqlen);

        struct protein_match match = {0};
        protein_match_init(&match);
        protein_match_set_frag(&match, step->seqlen, frag.str);
        protein_profile_state_name(step->state_id,
                                   protein_match_get_state_name(&match));

        if (!protein_state_is_mute(step->state_id))
        {
            rc = protein_profile_decode(work->prof, &frag, step->state_id,
                                        &codon);
            if (rc) return rc;
            protein_match_set_codon(&match, codon);
            protein_match_set_amino(&match, imm_gc_decode(1, codon));
        }
        if (idx > 0 && idx + 1 <= imm_path_nsteps(path))
        {
            rc = sched_prod_write_match_sep(work->prod_file.fp);
            if (rc) return rc;
        }
        if ((rc = sched_prod_write_match(work->prod_file.fp, &match)))
            return rc;
        start += step->seqlen;
    }
    rc = sched_prod_write_nl(work->prod_file.fp);
    return rc;
}
