#include "dcp/server.h"
#include "error.h"
#include "sql.h"

/* #define SQL_DB_NAME "deciphon" */

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
    int i;
    for (i = 0; i < argc; i++)
    {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

enum dcp_rc dcp_server_add_task(struct dcp_server *srv, struct dcp_task *tgt)
{
    char sql[] = "INSERT ";
    if (sqlite3_exec(srv->sql_db, sql, 0, 0, 0)) return DCP_RUNTIMEERROR;
    return DCP_SUCCESS;
}

enum dcp_rc dcp_server_init(struct dcp_server *srv)
{
    enum dcp_rc rc = DCP_SUCCESS;
    if ((rc = sql_open(srv))) return rc;
    if ((rc = sql_create(srv))) return rc;
    return rc;
}

void dcp_server_close(struct dcp_server *srv) { sql_close(srv); }
