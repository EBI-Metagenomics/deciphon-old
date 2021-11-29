#ifndef DCP_SRV_H
#define DCP_SRV_H

#include "dcp/export.h"
#include "dcp/job_state.h"
#include "dcp/limits.h"
#include "dcp/prod.h"
#include "dcp/rc.h"
#include <stdbool.h>
#include <stdint.h>

struct dcp_job;

DCP_API enum dcp_rc dcp_srv_open(char const *filepath, unsigned num_threads);
DCP_API enum dcp_rc dcp_srv_close(void);
DCP_API enum dcp_rc dcp_srv_add_db(char const *name, char const *filepath,
                                   int64_t *id);
DCP_API enum dcp_rc dcp_srv_submit_job(struct dcp_job *);
DCP_API enum dcp_rc dcp_srv_job_state(int64_t, enum dcp_job_state *);
DCP_API enum dcp_rc dcp_srv_run(bool single_run);
DCP_API enum dcp_rc dcp_srv_next_prod(int64_t job_id, int64_t *prod_id);
DCP_API struct dcp_prod const *dcp_srv_get_prod(void);

#endif
