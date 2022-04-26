#include "job.h"
#include "deciphon/db/profile_reader.h"
#include "deciphon/info.h"
#include "deciphon/logger.h"
#include "deciphon/rc.h"
#include "deciphon/sched/api.h"
#include "deciphon/version.h"
#include "deciphon/xfile.h"
#include "deciphon/xmath.h"
#include "hmm.h"
#include "imm/imm.h"
#include "scan.h"
#include "xomp.h"
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
    struct api_rc rerr = {0};

    enum rc rc = api_next_pend_job(&job->sched, &rerr);
    if (rc) return rc;
    if (rerr.rc) return erest(rerr.msg);

    rc = api_set_job_state(job->sched.id, SCHED_RUN, "", &rerr);
    if (rc) return rc;
    return rerr.rc ? erest(rerr.msg) : RC_OK;
}

#if 0
static inline void fail_job(int64_t job_id, char const *msg)
{
    struct api_error rerr = {0};
    api_set_job_state(job_id, SCHED_FAIL, msg, &rerr);
}
#endif

enum rc job_run(struct job *job)
{
    return job_run_func[job->sched.type](job->sched.id, job->num_threads);
}
