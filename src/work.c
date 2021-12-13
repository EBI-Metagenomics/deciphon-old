#include "work.h"
#include "compiler.h"
#include "db_reader.h"
#include "logger.h"
#include "prod.h"
#include "profile_reader.h"
#include "protein_match.h"
#include "protein_state.h"
#include "safe.h"
#include "sched_db.h"
#include "sched_job.h"
#include "sched_seq.h"
#include "utc.h"
#include "version.h"
#include "xfile.h"
#include "xmath.h"
#include <libgen.h>
#include <stdatomic.h>

struct hypothesis
{
    struct imm_task *task;
    struct imm_prod prod;
};

static struct work
{
    struct sched_job job;
    struct sched_seq sched_seq;
    struct imm_seq seq;
    struct hypothesis alt;
    struct hypothesis null;

    struct
    {
        struct sched_db sched_db;
        FILE *fp;
        struct db_reader reader;
    } db;

    struct profile_reader profile_reader;

    struct prod prod;
    struct xfile_tmp prod_file;

    atomic_bool failed;
    struct tok *tok;
} work = {0};

static inline struct imm_abc const *abc(void)
{
    return db_abc((struct db const *)&work.db);
}

enum rc write_product(struct work *work, struct task *task, unsigned match_id,
                      struct imm_seq seq);

void work_init(void) { atomic_store(&work.failed, false); }

enum rc work_next(void)
{
    enum rc rc = RC_DONE;
    if ((rc = sched_job_next_pending(&work.job))) return rc;
    if ((rc = sched_db_get_by_id(&work.db.sched_db, work.job.db_id))) return rc;
    return RC_NEXT;
}

static void cleanup_files(void)
{
    profile_reader_del(&work.profile_reader);
    xfile_tmp_del(&work.prod_file);
}

static enum rc open_readers(unsigned num_threads)
{
    enum rc rc = db_reader_open(&work.db.reader, work.db.fp);
    if (rc) return rc;

    struct db *db = db_reader_db(&work.db.reader);
    rc = profile_reader_setup(&work.profile_reader, db, num_threads);
    if (rc) db_reader_close(&work.db.reader);

    return rc;
}

static enum rc open_db(void)
{
    if (!(work.db.fp = fopen(work.db.sched_db.filepath, "rb")))
        return error(RC_IOERROR, "failed to open db");

    enum rc rc = db_reader_open(&work.db.reader, work.db.fp);
    if (rc) fclose(work.db.fp);
    return rc;
}

static enum rc close_db(void)
{
    enum rc rc = db_reader_close(&work.db.reader);
    if (rc) return rc;

    if (fclose(work.db.fp)) return error(RC_IOERROR, "failed to fclose");
    return rc;
}

static enum rc open_files(unsigned num_threads)
{
    enum rc rc = open_db();
    if (rc) return rc;

    struct db *db = db_reader_db(&work.db.reader);
    rc = profile_reader_setup(&work.profile_reader, db, num_threads);
    if (rc)
    {
        close_db();
        return rc;
    }

    rc = xfile_tmp_open(&work.prod_file);
    if (rc)
    {
        close_db();
        profile_reader_del(&work.profile_reader);
        return rc;
    }

    return rc;
}

static enum rc close_work(void)
{
    enum rc rc = db_reader_close(&work.db_reader);
    if ((rc = xfile_tmp_rewind(&work.prod_file))) goto cleanup;

    int64_t exec_ended = (int64_t)utc_now();
    if (work.failed)
    {
        rc = sched_job_set_error(work.job.id, "some error", exec_ended);
        if (rc) goto cleanup;
    }
    else
    {
        rc = sched_prod_add_from_tsv(work.prod_file.fp);
        if (rc) goto cleanup;
        rc = sched_job_set_done(work.job.id, exec_ended);
        if (rc) goto cleanup;
    }

cleanup:
    xfile_tmp_del(&work.prod_file);
    return rc;
}

static struct imm_str as_imm_str(void)
{
    return imm_str(array_data(work.sched_seq.data));
}

static struct imm_seq as_imm_seq(void) { return imm_seq(as_imm_str(), abc()); }

static bool check_sequence_alphabet(void)
{
    if (imm_abc_union_size(abc(), as_imm_str()) > 0)
    {
        warn(RC_ILLEGALARG, "out-of-alphabet characters");
        return false;
    }
    return true;
}

static enum rc reset_task(struct imm_task **task, struct imm_dp const *dp)
{
    if (*task && imm_task_reset(*task, dp))
        return error(RC_FAIL, "failed to reset task");

    if (!*task && !(*task = imm_task_new(dp)))
        return error(RC_FAIL, "failed to create task");

    return RC_DONE;
}

static enum rc setup_task(struct imm_task *task, struct imm_seq const *seq)
{
    if (imm_task_setup(task, seq))
        return error(RC_FAIL, "failed to setup task");
    return RC_DONE;
}

enum rc work_run(unsigned num_threads)
{
    enum rc rc = open_files(num_threads);
    if (rc) return rc;

    work.sched_seq.id = 0;
    while ((rc = sched_seq_next(work.job.id, &work.sched_seq)) == RC_NEXT)
    {
        if (!check_sequence_alphabet()) goto cleanup;
        struct imm_seq seq = as_imm_seq();

        unsigned match_id = 1;
        struct profile_reader *reader = &work.profile_reader;
        struct profile *prof = 0;
        while ((rc = profile_reader_next(reader, 0, &prof)) == RC_NEXT)
        {
            if ((rc = reset_task(&work.null.task, profile_null_dp(prof))))
                return rc;

            if ((rc = reset_task(&work.alt.task, profile_alt_dp(prof))))
                return rc;

            if ((rc = setup_task(work.null.task, &seq))) return rc;
            if ((rc = setup_task(work.alt.task, &seq))) return rc;

            imm_prod_reset(&work.null.prod);
            imm_prod_reset(&work.alt.prod);

            if (imm_dp_viterbi(profile_null_dp(prof), work.null.task,
                               &work.null.prod))
                return error(RC_FAIL, "failed to run viterbi");

            if (imm_dp_viterbi(profile_alt_dp(prof), work.alt.task,
                               &work.alt.prod))
                return error(RC_FAIL, "failed to run viterbi");

            double lrt = xmath_lrt(work.null.prod.loglik, work.alt.prod.loglik);

            if (lrt < 100.0f) continue;

            if ((rc = write_product(work, task, match_id, seq))) goto cleanup;
            match_id++;
        }
    }

    printf("\n");

    return close_work(work);

cleanup:
    atomic_store(&work.failed, true);
    close_work(work);
    return rc;
}

enum rc write_product(struct work *work, struct task *task, unsigned match_id,
                      struct imm_seq seq)
{
    enum rc rc = RC_DONE;
    struct profile *p = profile_reader_profile(&work.profile_reader, 0);
    /* TODO: generalized it */
    struct protein_profile *prof = (struct protein_profile *)p;
    struct imm_codon codon = imm_codon_any(prof->code->nuclt);

    struct metadata const *mt = &prof->super.metadata;

    sched_prod_set_job_id(&task->prod, work.job.id);
    sched_prod_set_seq_id(&task->prod, task->sched_seq.id);
    sched_prod_set_match_id(&task->prod, match_id);

    sched_prod_set_prof_name(&task->prod, mt->acc);
    sched_prod_set_abc_name(&task->prod, "dna_iupac");

    sched_prod_set_loglik(&task->prod, task->alt.prod.loglik);
    sched_prod_set_null_loglik(&task->prod, task->null.prod.loglik);

    sched_prod_set_prof_typeid(&task->prod, "pro");
    sched_prod_set_version(&task->prod, VERSION);

    rc = sched_prod_write_preamble(&task->prod, work.prod_file.fp);
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
            rc = protein_profile_decode(prof, &frag, step->state_id, &codon);
            if (rc) return rc;
            protein_match_set_codon(&match, codon);
            protein_match_set_amino(&match, imm_gc_decode(1, codon));
        }
        if (idx > 0 && idx + 1 <= imm_path_nsteps(path))
        {
            rc = sched_prod_write_match_sep(work.prod_file.fp);
            if (rc) return rc;
        }
        if ((rc = sched_prod_write_match(work.prod_file.fp, &match))) return rc;
        start += step->seqlen;
    }
    rc = sched_prod_write_nl(work.prod_file.fp);
    return rc;
}
