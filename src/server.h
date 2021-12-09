#ifndef SERVER_H
#define SERVER_H

#include "dcp_limits.h"
#include "job_state.h"
#include "prod.h"
#include "rc.h"
#include <stdbool.h>
#include <stdint.h>

struct job;

enum rc server_open(char const *filepath, unsigned num_threads);
enum rc server_close(void);
enum rc server_add_db(char const *filepath, int64_t *id);
enum rc server_submit_job(struct job *);
enum rc server_job_state(int64_t, enum job_state *);
enum rc server_run(bool single_run);
enum rc server_next_prod(int64_t job_id, int64_t *prod_id);
struct prod const *server_get_prod(void);

#endif
