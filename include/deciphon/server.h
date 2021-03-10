#ifndef DECIPHON_SERVER_H
#define DECIPHON_SERVER_H

#include "deciphon/export.h"
#include <inttypes.h>

struct dcp_result;
struct dcp_server;
struct dcp_task;

DCP_API struct dcp_server* dcp_server_create(char const* filepath);
DCP_API void               dcp_server_destroy(struct dcp_server const* server);
DCP_API double             dcp_server_elapsed(struct dcp_server const* server);
DCP_API void               dcp_server_scan(struct dcp_server* server, struct dcp_task* task);

#endif
