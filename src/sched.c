#include "sched.h"
#include "dcp.h"
#include "dcp/job.h"
#include "dcp/seq.h"
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

static struct
{
    char const *begin;
    struct
    {
        char const *job;
        char const *seq;
    } submit;
    char const *end;
} const sched_stmt = {
    .begin = "BEGIN TRANSACTION;",
    .submit = {.job = " INSERT INTO job (multi_hits, hmmer3_compat, db_id, "
                      "submission) VALUES (?, ?, ?, ?) RETURNING id;",
               .seq = "INSERT INTO seq (data, job_id) VALUES (?, ?);"},
    .end = "COMMIT TRANSACTION;"};

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

#define ERROR_RESET(n) error(DCP_RUNTIMEERROR, "failed to reset " n " stmt")
#define ERROR_STEP(n) error(DCP_RUNTIMEERROR, "failed to exec " n " stmt")
#define ERROR_BIND(n) error(DCP_RUNTIMEERROR, "failed to bind " n)

static enum dcp_rc begin_transaction(struct sched *sched)
{
    if (sqlite3_reset(sched->stmt.begin)) return ERROR_RESET("begin");
    if (sqlite3_step(sched->stmt.begin) != SQLITE_DONE)
        return ERROR_STEP("begin");
    return DCP_SUCCESS;
}

static enum dcp_rc end_transaction(struct sched *sched)
{
    if (sqlite3_reset(sched->stmt.end)) return ERROR_RESET("end");
    if (sqlite3_step(sched->stmt.end) != SQLITE_DONE) return ERROR_STEP("end");
    return DCP_SUCCESS;
}

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

    enum dcp_rc rc = DCP_SUCCESS;
    sched->stmt.begin = NULL;
    sched->stmt.submit.job = NULL;
    sched->stmt.submit.seq = NULL;
    sched->stmt.end = NULL;

    if (sqlite3_prepare_v2(sched->db, sched_stmt.begin, -1, &sched->stmt.begin,
                           NULL))
    {
        rc = error(DCP_RUNTIMEERROR, "failed to prepare begin stmt");
        goto cleanup;
    }

    if (sqlite3_prepare_v2(sched->db, sched_stmt.submit.job, -1,
                           &sched->stmt.submit.job, NULL))
    {
        rc = error(DCP_RUNTIMEERROR, "failed to prepare submit.job stmt");
        goto cleanup;
    }

    if (sqlite3_prepare_v2(sched->db, sched_stmt.submit.seq, -1,
                           &sched->stmt.submit.seq, NULL))
    {
        rc = error(DCP_RUNTIMEERROR, "failed to prepare submit.seq stmt");
        goto cleanup;
    }

    if (sqlite3_prepare_v2(sched->db, sched_stmt.end, -1, &sched->stmt.end,
                           NULL))
    {
        rc = error(DCP_RUNTIMEERROR, "failed to prepare end stmt");
        goto cleanup;
    }

    return rc;

cleanup:
    sqlite3_finalize(sched->stmt.end);
    sqlite3_finalize(sched->stmt.submit.seq);
    sqlite3_finalize(sched->stmt.submit.job);
    sqlite3_finalize(sched->stmt.begin);
    sqlite3_close(sched->db);
    return rc;
}

enum dcp_rc sched_close(struct sched *sched)
{
    sqlite3_finalize(sched->stmt.end);
    sqlite3_finalize(sched->stmt.submit.seq);
    sqlite3_finalize(sched->stmt.submit.job);
    sqlite3_finalize(sched->stmt.begin);
    if (sqlite3_close(sched->db)) return close_error();
    return DCP_SUCCESS;
}

static enum dcp_rc submit_job(sqlite3_stmt *stmt, struct dcp_job *job,
                              uint64_t db_id, uint64_t *job_id)
{
    enum dcp_rc rc = DCP_SUCCESS;
    if (sqlite3_reset(stmt))
    {
        rc = ERROR_RESET("submit job");
        goto cleanup;
    }
    if (sqlite3_bind_int(stmt, 1, job->multi_hits))
    {
        rc = ERROR_BIND("multi_hits");
        goto cleanup;
    }
    if (sqlite3_bind_int(stmt, 2, job->hmmer3_compat))
    {
        rc = ERROR_BIND("hmmer3_compat");
        goto cleanup;
    }
    if (sqlite3_bind_int64(stmt, 3, (sqlite3_int64)db_id))
    {
        rc = ERROR_BIND("db_id");
        goto cleanup;
    }
    sqlite3_int64 utc = (sqlite3_int64)dcp_utc_now();
    if (sqlite3_bind_int64(stmt, 4, utc))
    {
        rc = ERROR_BIND("submission");
        goto cleanup;
    }
    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        rc = ERROR_STEP("job");
        goto cleanup;
    }
    sqlite3_int64 id64 = sqlite3_column_int64(stmt, 0);
    assert(id64 > 0);
    *job_id = (uint64_t)id64;

    if (sqlite3_step(stmt) != SQLITE_DONE) rc = ERROR_STEP("job");

cleanup:
    return rc;
}

static enum dcp_rc add_seq(sqlite3_stmt *stmt, char const *seq,
                           sqlite3_int64 job_id)
{
    enum dcp_rc rc = DCP_SUCCESS;
    if (sqlite3_reset(stmt))
    {
        rc = ERROR_RESET("add seq");
        goto cleanup;
    }
    if (sqlite3_bind_text(stmt, 1, seq, -1, NULL))
    {
        rc = ERROR_BIND("seq");
        goto cleanup;
    }
    if (sqlite3_bind_int64(stmt, 2, job_id))
    {
        rc = ERROR_BIND("job_id");
        goto cleanup;
    }
    if (sqlite3_step(stmt) != SQLITE_DONE) rc = ERROR_STEP("seq");

cleanup:
    return rc;
}

enum dcp_rc sched_submit(struct sched *sched, struct dcp_job *job,
                         uint64_t db_id, uint64_t *job_id)
{
    enum dcp_rc rc = DCP_SUCCESS;
    if ((rc = begin_transaction(sched))) goto cleanup;

    if ((rc = submit_job(sched->stmt.submit.job, job, db_id, job_id)))
        goto cleanup;

    struct cco_iter iter = cco_queue_iter(&job->seqs);
    struct dcp_seq *seq = NULL;
    cco_iter_for_each_entry(seq, &iter, node)
    {

        sqlite3_int64 id = (sqlite3_int64)*job_id;
        if (add_seq(sched->stmt.submit.seq, seq->data, id)) goto cleanup;
    }
    rc = end_transaction(sched);

cleanup:
    return rc;
}

enum dcp_rc sched_add_db(struct sched *sched, char const *filepath,
                         uint64_t *id)
{
    enum dcp_rc rc = DCP_SUCCESS;
    static char const sql[] =
        "INSERT INTO db (filepath) VALUES (?) RETURNING id;";
    sqlite3_stmt *stmt = NULL;

    if (sqlite3_prepare_v2(sched->db, sql, -1, &stmt, NULL))
    {
        rc = error(DCP_RUNTIMEERROR, "failed to prepare sqlite");
        goto cleanup;
    }
    if (sqlite3_bind_text(stmt, 1, filepath, -1, NULL))
    {
        rc = error(DCP_RUNTIMEERROR, "failed to bind filepath");
        goto cleanup;
    }
    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        rc = error(DCP_RUNTIMEERROR, "failed to add db row");
        goto cleanup;
    }

    sqlite3_int64 id64 = sqlite3_column_int64(stmt, 0);
    assert(id64 > 0);
    *id = (uint64_t)id64;

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        rc = error(DCP_RUNTIMEERROR, "failed to add db");
        goto cleanup;
    }

cleanup:
    if (sqlite3_finalize(stmt))
        rc = error(DCP_RUNTIMEERROR, "failed to finalize sqlite");

    return rc;
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
