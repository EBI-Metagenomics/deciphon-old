#ifndef DCP_SRV_H
#define DCP_SRV_H

#include "dcp/export.h"
#include "dcp/job_state.h"
#include "dcp/limits.h"
#include "dcp/rc.h"
#include <stdbool.h>
#include <stdint.h>

struct dcp_job;
struct dcp_srv;

DCP_API struct dcp_srv *dcp_srv_open(char const *filepath);
DCP_API enum dcp_rc dcp_srv_close(struct dcp_srv *);
DCP_API enum dcp_rc dcp_srv_add_db(struct dcp_srv *, char const *name,
                                   char const *filepath, int64_t *id);
DCP_API enum dcp_rc dcp_srv_submit_job(struct dcp_srv *, struct dcp_job *);
DCP_API enum dcp_rc dcp_srv_job_state(struct dcp_srv *, int64_t,
                                      enum dcp_job_state *);
DCP_API enum dcp_rc dcp_srv_run(struct dcp_srv *, bool blocking);
DCP_API enum dcp_rc dcp_srv_next_prod(struct dcp_srv *, int64_t job_id,
                                      int64_t *prod_id);

DCP_API void dcp_srv_prod_seq_id(struct dcp_srv *srv, int64_t prod_id,
                                 int64_t *seq_id);
DCP_API void dcp_srv_prod_match_id(struct dcp_srv *srv, int64_t prod_id,
                                   int64_t *match_id);
DCP_API void dcp_srv_prod_prof_name(struct dcp_srv *srv, int64_t prod_id,
                                    char prof_name[DCP_PROF_NAME_SIZE]);
DCP_API void dcp_srv_prod_abc_name(struct dcp_srv *srv, int64_t prod_id,
                                   char abc_name[DCP_ABC_NAME_SIZE]);
DCP_API void dcp_srv_prod_loglik(struct dcp_srv *srv, int64_t prod_id,
                                 double *loglik);
DCP_API void dcp_srv_prod_null_loglik(struct dcp_srv *srv, int64_t prod_id,
                                      double *null_loglik);
DCP_API void dcp_srv_prod_model(struct dcp_srv *srv, int64_t prod_id,
                                char model[DCP_MODEL_SIZE]);
DCP_API void dcp_srv_prod_version(struct dcp_srv *srv, int64_t prod_id,
                                  char version[DCP_VERSION_SIZE]);
DCP_API void dcp_srv_prod_match_data(struct dcp_srv *srv, int64_t prod_id,
                                     char match_data[DCP_MATCH_DATA_SIZE]);

#endif
