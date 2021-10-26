#include "dcp/server.h"
#include "dcp/db.h"
#include "dcp/prof_types.h"
#include "error.h"
#include "jobs.h"

enum dcp_rc dcp_server_add_db(struct dcp_server *srv, unsigned user_id,
                              char const *name, char const *filepath)
{
    FILE *fd = fopen(filepath, "rb");
    if (!fd) return error(DCP_IOERROR, "failed to open file");

    enum dcp_prof_typeid typeid = 0;
    enum dcp_rc rc = dcp_db_fetch_prof_type(fd, &typeid);
    if (rc) goto cleanup;

    char type[4] = {0};
    if (typeid == DCP_STD_PROFILE)
        strcpy(type, "std");
    else if (typeid == DCP_PRO_PROFILE)
        strcpy(type, "pro");
    else
    {
        rc = error(DCP_RUNTIMEERROR, "unknown profile type");
        goto cleanup;
    }

    fclose(fd);
    return jobs_add_db(&srv->jobs, user_id, name, filepath, "xxh3", type);

cleanup:
    fclose(fd);
    return rc;
}

enum dcp_rc dcp_server_add_job(struct dcp_server *srv, unsigned user_id,
                               char const *sid, struct dcp_job *job)
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
    if ((rc = jobs_open(&srv->jobs, jobs_fp))) goto cleanup;

cleanup:
    return rc;
}

enum dcp_rc dcp_server_close(struct dcp_server *srv)
{
    return jobs_close(&srv->jobs);
}
