#ifndef SQL_H
#define SQL_H

enum dcp_rc;
struct dcp_server;

enum dcp_rc sql_create(struct dcp_server *srv);
enum dcp_rc sql_open(struct dcp_server *srv);
void sql_close(struct dcp_server *srv);

#endif
