#include "dcp/server.h"
#include "error.h"
#include "sql.h"
#include <uriparser/Uri.h>

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

enum dcp_rc dcp_server_init(struct dcp_server *srv, char const *db_uri)
{
    UriUriA uri = {0};
    enum dcp_rc rc = DCP_SUCCESS;

    if (uriParseSingleUriA(&uri, db_uri, 0))
        return error(DCP_ILLEGALARG, "failed to parse uri");

    if (uriNormalizeSyntaxExA(&uri, uriNormalizeSyntaxMaskRequiredA(&uri)))
    {
        rc = error(DCP_ILLEGALARG, "failed to normalise uri");
        goto cleanup;
    }

    if (strncmp(uri.scheme.first, "file:", 5))
    {
        rc = error(DCP_ILLEGALARG, "unknown uri scheme");
        goto cleanup;
    }
    if ((rc = sql_setup(srv, &uri))) goto cleanup;

cleanup:
    uriFreeUriMembersA(&uri);
    return rc;
}

void dcp_server_close(struct dcp_server *srv) { sql_close(srv); }
