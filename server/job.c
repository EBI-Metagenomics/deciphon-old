#include "job.h"
#include "deciphon/db/profile_reader.h"
#include "deciphon/info.h"
#include "deciphon/logger.h"
#include "deciphon/rc.h"
#include "deciphon/server/api.h"
#include "deciphon/version.h"
#include "deciphon/xfile.h"
#include "deciphon/xmath.h"
#include "imm/imm.h"
#include "xomp.h"
#include <stdatomic.h>
#include <string.h>

void job_init(struct job *job, unsigned num_threads)
{
    sched_job_init(&job->sched_job);
    job->num_threads = num_threads;
}

enum rc job_next(struct job *job)
{
    struct api_error rerr = {0};

    enum rc rc = api_next_pend_job(&job->sched_job, &rerr);
    if (rc) return rc;
    if (rerr.rc) return erest(rerr.msg);

    rc = api_set_job_state(job->sched_job.id, SCHED_RUN, "", &rerr);
    if (rc) return rc;
    return rerr.rc ? erest(rerr.msg) : RC_OK;
}

static inline void fail_job(int64_t job_id, char const *msg)
{
    struct api_error rerr = {0};
    api_set_job_state(job_id, SCHED_FAIL, msg, &rerr);
}

enum rc job_run(struct job *job) {}
