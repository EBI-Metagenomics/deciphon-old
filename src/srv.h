#ifndef SRV_H
#define SRV_H

#include "dcp_limits.h"
#include "job_state.h"
#include "prod.h"
#include "rc.h"
#include <stdbool.h>
#include <stdint.h>

struct dcp_job;

enum dcp_rc dcp_srv_open(char const *filepath, unsigned num_threads);
enum dcp_rc dcp_srv_close(void);
enum dcp_rc dcp_srv_add_db(char const *name, char const *filepath, int64_t *id);
enum dcp_rc dcp_srv_submit_job(struct dcp_job *);
enum dcp_rc dcp_srv_job_state(int64_t, enum dcp_job_state *);
enum dcp_rc dcp_srv_run(bool single_run);
enum dcp_rc dcp_srv_next_prod(int64_t job_id, int64_t *prod_id);
struct prod const *dcp_srv_get_prod(void);

#endif
