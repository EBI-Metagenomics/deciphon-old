#ifndef DCP_SERVER_H
#define DCP_SERVER_H

#include "common/limits.h"
#include "common/rc.h"
#include "imm/imm.h"
#include "job_state.h"
#include "prod.h"
#include <stdbool.h>
#include <stdint.h>

// enum sched_job_state;
// struct job;
// struct sched_prod;

// enum rc server_open(char const *filepath, unsigned num_threads);
// enum rc server_close(void);
// enum rc server_add_db(char const *filepath, int64_t *id);
// enum rc server_submit_job(struct job *);
// enum rc server_job_state(int64_t job_id, enum sched_job_state *state);
enum rc server_run(bool single_run, unsigned num_threads, char const *url);
void server_set_lrt_threshold(imm_float lrt);

// enum rc server_get_sched_job(struct sched_job *job);
// enum rc server_next_sched_prod(struct sched_job const *job,
//                                struct sched_prod *prod);

#endif
