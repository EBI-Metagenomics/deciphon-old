#include "job_state.h"
#include <stdlib.h>
#include <string.h>

enum job_state job_state_resolve(char const *state)
{
    if (strcmp("pend", state) == 0)
        return JOB_PEND;
    else if (strcmp("run", state) == 0)
        return JOB_RUN;
    else if (strcmp("done", state) == 0)
        return JOB_DONE;
    else if (strcmp("fail", state) == 0)
        return JOB_FAIL;

    exit(EXIT_FAILURE);
    return 0;
}
