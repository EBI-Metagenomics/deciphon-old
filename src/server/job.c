#include "job.h"
#include "core/logging.h"
#include "core/rc.h"
#include "core/xfile.h"
#include "core/xmath.h"
#include "db/profile_reader.h"
#include "deciphon/sched/api.h"
#include "hmm.h"
#include "imm/imm.h"
#include "scan.h"
#include "xomp.h"
#include <stdarg.h>
#include <stdatomic.h>
#include <string.h>

typedef enum rc (*job_run_func_t)(int64_t job_id, unsigned num_threads);

static job_run_func_t job_run_func[] = {scan_run, hmm_press};

void job_init(struct job *job, unsigned num_threads)
{
    sched_job_init(&job->sched);
    job->num_threads = num_threads;
}

enum rc job_next(struct job *job)
{
    // struct api_error err = {0};

    enum rc rc = api_job_next_pend(&job->sched);
    if (rc) return rc;
    // if (err.rc) return eapi(err);

    rc = api_job_set_state(job->sched.id, SCHED_RUN, "");
    return rc;
    // if (rc) return rc;
    // return err.rc ? eapi(err) : RC_OK;
}

enum rc job_run(struct job *job)
{
    return job_run_func[job->sched.type](job->sched.id, job->num_threads);
}

void job_set_fail(int64_t job_id, char const *fmt, ...)
{
    char msg[SCHED_JOB_ERROR_SIZE] = {0};
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof msg, fmt, args);
    va_end(args);

    warn("Job failed with message `%s`", msg);

    api_job_set_state(job_id, SCHED_FAIL, msg);
}
