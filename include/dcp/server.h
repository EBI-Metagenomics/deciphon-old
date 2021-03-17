#ifndef DCP_SERVER_H
#define DCP_SERVER_H

#include "deciphon/export.h"
#include <inttypes.h>

struct dcp_server;
struct dcp_task;

DCP_API struct dcp_server*         dcp_server_create(char const* filepath);
DCP_API void                       dcp_server_destroy(struct dcp_server const* server);
DCP_API struct dcp_metadata const* dcp_server_metadata(struct dcp_server const* server, uint32_t profid);
DCP_API uint32_t                   dcp_server_nprofiles(struct dcp_server const* server);
DCP_API void                       dcp_server_scan(struct dcp_server* server, struct dcp_task* task);

#endif
