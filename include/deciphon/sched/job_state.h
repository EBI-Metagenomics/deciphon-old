#ifndef DECIPHON_SCHED_JOB_STATE_H
#define DECIPHON_SCHED_JOB_STATE_H

#include "deciphon/sched/sched.h"

char const *job_state_decode(enum sched_job_state state);
bool job_state_encode(char const *str, enum sched_job_state *state);

#endif
