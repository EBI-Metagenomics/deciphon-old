#ifndef DCP_SERVER_H
#define DCP_SERVER_H

#include "dcp/export.h"
#include "dcp/rc.h"
#include <sqlite3.h>
#include <stdint.h>
#include <stdio.h>

/* struct dcp_result; */
/* struct dcp_server; */
struct dcp_task;

struct dcp_server
{
    sqlite3 *sql_db;
};

DCP_API enum dcp_rc dcp_server_add_task(struct dcp_server *, struct dcp_task *);
DCP_API enum dcp_rc dcp_server_init(struct dcp_server *);
DCP_API void dcp_server_close(struct dcp_server *);
/* DCP_API int dcp_server_destroy(struct dcp_server *server); */
/* DCP_API void dcp_server_free_result(struct dcp_server *server, */
/*                                     struct dcp_result const *result); */
/* DCP_API void dcp_server_free_task(struct dcp_server *server, */
/*                                   struct dcp_task *task); */
/* DCP_API int dcp_server_join(struct dcp_server *server); */
/* DCP_API struct dcp_metadata const * */
/* dcp_server_metadata(struct dcp_server const *server, uint32_t profid); */
/* DCP_API uint32_t dcp_server_nprofiles(struct dcp_server const *server); */
/* DCP_API int dcp_server_start(struct dcp_server *server); */
/* DCP_API void dcp_server_stop(struct dcp_server *server); */

#endif
