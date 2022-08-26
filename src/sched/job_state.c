#include "deciphon/sched/sched.h"
#include <assert.h>
#include <string.h>

static char const strings[][5] = {"pend", "run", "done", "fail"};

char const *job_state_decode(enum sched_job_state state)
{
    if (state < SCHED_PEND || state > SCHED_FAIL) return 0;
    return strings[state];
}

bool job_state_encode(char const *str, enum sched_job_state *state)
{
    for (int i = 0; i < 4; ++i)
    {
        if (!strcmp(strings[i], str))
        {
            *state = i;
            return true;
        }
    }
    return false;
}
