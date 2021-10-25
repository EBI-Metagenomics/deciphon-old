#include "dcp/server.h"
#include "error.h"
#include "jobs.h"

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

enum dcp_rc dcp_server_add_db(struct dcp_server *srv, char const *filepath)
{
    return jobs_add_db(&srv->jobs, filepath);
}

enum dcp_rc dcp_server_add_task(struct dcp_server *srv, struct dcp_task *tgt)
{
#if 0
    char sql[] = "INSERT ";
    if (sqlite3_exec(srv->sql_db, sql, 0, 0, 0)) return DCP_RUNTIMEERROR;
    return DCP_SUCCESS;
#endif
    return DCP_SUCCESS;
}

enum dcp_rc dcp_server_setup(struct dcp_server *srv, char const *jobs_fp)
{
    enum dcp_rc rc = DCP_SUCCESS;
    if ((rc = jobs_setup(&srv->jobs, jobs_fp))) goto cleanup;

cleanup:
    return rc;
}

enum dcp_rc dcp_server_close(struct dcp_server *srv)
{
    return jobs_close(&srv->jobs);
}
