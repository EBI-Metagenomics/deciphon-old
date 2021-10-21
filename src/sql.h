#ifndef SQL_H
#define SQL_H

enum dcp_rc;
struct dcp_server;

enum dcp_rc sql_setup(struct dcp_server *srv, char const *filepath);
void sql_close(struct dcp_server *srv);

#endif
