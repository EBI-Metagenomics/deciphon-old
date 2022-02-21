#ifndef DCP_JOB_STATE_H
#define DCP_JOB_STATE_H

enum job_state
{
    JOB_PEND,
    JOB_RUN,
    JOB_DONE,
    JOB_FAIL
};

enum job_state job_state_resolve(char const *state);

#endif
