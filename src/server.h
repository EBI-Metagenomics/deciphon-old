#ifndef SERVER_H
#define SERVER_H

#include "dcp_limits.h"
#include "job_state.h"
#include "prod.h"
#include "rc.h"
#include <stdbool.h>
#include <stdint.h>

enum sched_job_state;
struct job;

enum rc server_open(char const *filepath, unsigned num_threads);
enum rc server_close(void);
enum rc server_add_db(char const *filepath, int64_t *id);
enum rc server_submit_job(struct job *);
enum rc server_job_state(int64_t job_id, enum sched_job_state *state);
enum rc server_run(bool single_run);
void server_set_lrt_threshold(imm_float lrt);
/* enum rc server_next_prod(int64_t job_id, int64_t *prod_id); */
/* struct prod const *server_get_prod(void); */

#endif
