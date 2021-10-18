#include "dcp/server.h"
#include "error.h"

#define SQL_DB_NAME "deciphon"

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

enum dcp_rc open_sql_db(struct dcp_server *srv);
void close_sql_db(struct dcp_server *srv);

enum dcp_rc dcp_server_init(struct dcp_server *srv)
{
    enum dcp_rc rc = DCP_SUCCESS;
    if (!(rc = open_sql_db(srv))) return rc;
    return rc;
}

enum dcp_rc open_sql_db(struct dcp_server *srv)
{
    if (sqlite3_open(SQL_DB_NAME, &srv->sql_db))
    {
        sqlite3_close(srv->sql_db);
        return error(DCP_RUNTIMEERROR, "failed to open database");
    }
    return DCP_SUCCESS;
}

void close_sql_db(struct dcp_server *srv) { sqlite3_close(srv->sql_db); }
