#include "deciphon/sched/sched.h"
#include <assert.h>
#include <string.h>

char const *job_state_decode(enum sched_job_state state)
{
    if (state == SCHED_PEND) return "pend";
    if (state == SCHED_RUN) return "run";
    if (state == SCHED_DONE) return "done";
    if (state == SCHED_FAIL) return "fail";
    assert(0);
    return "";
}

enum sched_job_state job_state_encode(char const *state)
{
    if (strcmp("pend", state) == 0)
        return SCHED_PEND;
    else if (strcmp("run", state) == 0)
        return SCHED_RUN;
    else if (strcmp("done", state) == 0)
        return SCHED_DONE;
    else if (strcmp("fail", state) == 0)
        return SCHED_FAIL;

    assert(0);
    return 0;
}
