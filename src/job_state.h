#ifndef JOB_STATE_H
#define JOB_STATE_H

enum dcp_job_state
{
    DCP_JOB_PEND,
    DCP_JOB_RUN,
    DCP_JOB_DONE,
    DCP_JOB_FAIL
};

enum dcp_job_state job_state_resolve(char const *state);

#endif
