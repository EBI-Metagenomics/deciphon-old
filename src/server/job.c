#include "job.h"
#include "deciphon/core/logging.h"
#include "deciphon/core/rc.h"
#include "deciphon/core/xfile.h"
#include "deciphon/core/xmath.h"
#include "deciphon/db/profile_reader.h"
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
    struct api_rc api_rc = {0};

    enum rc rc = api_next_pend_job(&job->sched, &api_rc);
    if (rc) return rc;
    if (api_rc.rc) return eapi(api_rc);

    rc = api_set_job_state(job->sched.id, SCHED_RUN, "", &api_rc);
    if (rc) return rc;
    return api_rc.rc ? eapi(api_rc) : RC_OK;
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

    struct api_rc discard = {0};
    api_set_job_state(job_id, SCHED_FAIL, msg, &discard);
}
