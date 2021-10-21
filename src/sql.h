#ifndef SQL_H
#define SQL_H

#include <uriparser/Uri.h>

enum dcp_rc;
struct dcp_server;

enum dcp_rc sql_setup(struct dcp_server *srv, UriUriA const *uri);
void sql_close(struct dcp_server *srv);

#endif
