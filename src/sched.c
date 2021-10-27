#include "sched.h"
#include "dcp.h"
#include "dcp/server.h"
#include "dcp_file.h"
#include "error.h"
#include "imm/imm.h"
#include "sched_job.h"
#include "schema.h"
#include "sqldiff.h"
#include "third-party/base64.h"
#include <assert.h>
#include <inttypes.h>
#include <sqlite3.h>
#include <stdio.h>

static_assert(SQLITE_VERSION_NUMBER >= 3035000, "We need RETURNING statement");

#define SQL_SIZE 512

static_assert(IMM_ABC_MAX_SIZE == 31, "IMM_ABC_MAX_SIZE == 31");
static_assert(IMM_SYM_SIZE == 94, "IMM_SYM_SIZE == 94");

static enum dcp_rc check_integrity(char const *filepath, bool *ok);
static enum dcp_rc emerge_db(char const *filepath);
static enum dcp_rc is_empty(char const *filepath, bool *empty);
static enum dcp_rc touch_db(char const *filepath);

static inline enum dcp_rc open_error(sqlite3 *db)
{
    sqlite3_close(db);
    return error(DCP_RUNTIMEERROR, "failed to open jobs database");
}

static inline enum dcp_rc close_error(void)
{
    return error(DCP_RUNTIMEERROR, "failed to close jobs database");
}

static inline enum dcp_rc exec_sql(sqlite3 *db, char const sql[SQL_SIZE])
{
    if (sqlite3_exec(db, sql, NULL, NULL, NULL))
        return error(DCP_RUNTIMEERROR, "failed to exec sql");
    return DCP_SUCCESS;
}

static enum dcp_rc insert_into_job(sqlite3 *db, bool multiple_hits,
                                   bool hmmer3_compat, uint64_t db_id,
                                   dcp_utc submission)
{
    char sql[SQL_SIZE] = {0};
    int rc = snprintf(sql, ARRAY_SIZE(sql),
                      "INSERT INTO job "
                      "(multiple_hits, hmmer3_compat, db_id, submission)"
                      " VALUES (%d, %d, %" PRIu64 ", %" PRIu64 ");",
                      multiple_hits, hmmer3_compat, db_id, submission);
    assert(rc >= 0);
    return exec_sql(db, sql);
}

#if 0
static enum dcp_rc insert_into_db(sqlite3 *db, char const *filepath)
{
    char sql[SQL_SIZE] = {0};
    int rc = snprintf(sql, ARRAY_SIZE(sql),
                      "INSERT INTO db (filepath)"
                      " VALUES ('%s');",
                      filepath);
    sqlite3_stmt *pStmt;
    sqlite3_prepare_v2();
    sqlite3_prepare_v2(db, "select * from expenses", -1, &stmt, NULL);
    assert(rc >= 0);
    return exec_sql(db, sql);
}
#endif

enum dcp_rc sched_setup(char const *filepath)
{
    enum dcp_rc rc = touch_db(filepath);
    if (rc) return rc;

    bool empty = false;
    if ((rc = is_empty(filepath, &empty))) return rc;

    if (empty && (rc = emerge_db(filepath))) return rc;

    bool ok = false;
    if ((rc = check_integrity(filepath, &ok))) return rc;
    if (!ok) return error(DCP_RUNTIMEERROR, "damaged jobs database");

    return rc;
}

enum dcp_rc sched_open(struct sched *sched, char const *filepath)
{
    if (sqlite3_open(filepath, &sched->db)) return open_error(sched->db);
    return DCP_SUCCESS;
}

enum dcp_rc sched_close(struct sched *sched)
{
    if (sqlite3_close(sched->db)) return close_error();
    return DCP_SUCCESS;
}

enum dcp_rc sched_submit(struct sched *sched, struct sched_job *job)
{
    return insert_into_job(sched->db, job->multiple_hits, job->hmmer3_compat,
                           job->db_id, dcp_utc_now());
}

enum dcp_rc sched_add_db(struct sched *sched, char const *filepath,
                         uint64_t *id)
{
    char *zSQL = sqlite3_mprintf("INSERT INTO db (filepath) VALUES (%Q)"
                                 "RETURNING id;",
                                 filepath);
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(sched->db, zSQL, 0, &stmt, NULL);
    while ((rc = sqlite3_step (stmt) == SQLITE_ROW)) {
    }
    /* int sqlite3_step(sqlite3_stmt*); */
    rc = sqlite3_finalize(stmt);
    return DCP_SUCCESS;
}

static enum dcp_rc create_ground_truth_db(struct file_tmp *tmp)
{
    enum dcp_rc rc = DCP_SUCCESS;
    if ((rc = file_tmp_mk(tmp))) return rc;
    if ((rc = touch_db(tmp->path))) return rc;
    if ((rc = emerge_db(tmp->path))) return rc;
    return rc;
}

static enum dcp_rc check_integrity(char const *filepath, bool *ok)
{
    struct file_tmp tmp = FILE_TMP_INIT();
    enum dcp_rc rc = DCP_SUCCESS;

    if ((rc = create_ground_truth_db(&tmp))) return rc;
    if ((rc = sqldiff_compare(filepath, tmp.path, ok))) goto cleanup;

cleanup:
    file_tmp_rm(&tmp);
    return rc;
}

static enum dcp_rc emerge_db(char const *filepath)
{
    sqlite3 *db = NULL;
    if (sqlite3_open(filepath, &db)) return open_error(db);

    enum dcp_rc rc = exec_sql(db, (char const *)schema_sql);
    if (rc) return rc;
    if (sqlite3_close(db)) return close_error();

    return DCP_SUCCESS;
}

static int is_empty_cb(void *empty, int argc, char **argv, char **cols)
{
    *((bool *)empty) = false;
    return 0;
}

static enum dcp_rc is_empty(char const *filepath, bool *empty)
{
    sqlite3 *db = NULL;
    if (sqlite3_open(filepath, &db)) return open_error(db);

    *empty = true;
    static char const *const sql = "SELECT name FROM sqlite_master;";

    if (sqlite3_exec(db, sql, is_empty_cb, empty, 0))
    {
        sqlite3_close(db);
        return error(DCP_RUNTIMEERROR, "failed to check if jobs db is empty");
    }

    if (sqlite3_close(db)) return close_error();
    return DCP_SUCCESS;
}

static enum dcp_rc touch_db(char const *filepath)
{
    sqlite3 *db = NULL;
    if (sqlite3_open(filepath, &db)) return open_error(db);
    if (sqlite3_close(db)) return close_error();
    return DCP_SUCCESS;
}
