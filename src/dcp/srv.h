#ifndef DCP_SRV_H
#define DCP_SRV_H

#include "dcp/export.h"
#include "dcp/job_state.h"
#include "dcp/rc.h"
#include <stdbool.h>
#include <stdint.h>

struct dcp_job;
struct dcp_srv;

DCP_API struct dcp_srv *dcp_srv_open(char const *filepath);
DCP_API enum dcp_rc dcp_srv_close(struct dcp_srv *);
DCP_API enum dcp_rc dcp_srv_add_db(struct dcp_srv *, char const *, int64_t *);
DCP_API enum dcp_rc dcp_srv_submit_job(struct dcp_srv *, struct dcp_job *,
                                       int64_t, int64_t *);
DCP_API enum dcp_rc dcp_srv_job_state(struct dcp_srv *, int64_t,
                                      enum dcp_job_state *);
DCP_API enum dcp_rc dcp_srv_run(struct dcp_srv *, bool blocking);

#endif