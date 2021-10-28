#ifndef DCP_SERVER_H
#define DCP_SERVER_H

#include "dcp/export.h"
#include "dcp/rc.h"
#include <stdint.h>

struct dcp_job;
struct dcp_server;

DCP_API struct dcp_server *dcp_server_open(char const *filepath);
DCP_API enum dcp_rc dcp_server_close(struct dcp_server *);
DCP_API enum dcp_rc dcp_server_add_db(struct dcp_server *, char const *,
                                      uint64_t *);
DCP_API enum dcp_rc dcp_server_submit_job(struct dcp_server *, struct dcp_job *,
                                          uint64_t);

#endif
