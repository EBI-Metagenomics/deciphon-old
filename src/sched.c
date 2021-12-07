#include "sched.h"
#include "compiler.h"
#include "job.h"
#include "logger.h"
#include "rc.h"
#include "sched_db.h"
#include "sched_job.h"
#include "sched_prod.h"
#include "sched_schema.h"
#include "sched_seq.h"
#include "sqldiff.h"
#include "utc.h"
#include "xfile.h"
#include "xsql.h"
#include <assert.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static struct sqlite3 *sqlite3_db = NULL;

static_assert(SQLITE_VERSION_NUMBER >= 3035000, "We need RETURNING statement");

enum rc check_integrity(char const *filepath, bool *ok);
enum rc create_ground_truth_db(char *filepath);
enum rc emerge_db(char const *filepath);
enum rc is_empty(char const *filepath, bool *empty);
enum rc submit_job(struct sqlite3_stmt *, struct sched_job *, int64_t db_id,
                   int64_t *job_id);
enum rc touch_db(char const *filepath);

enum rc sched_setup(char const filepath[DCP_PATH_SIZE])
{
    enum rc rc = touch_db(filepath);
    if (rc) return rc;

    bool empty = false;
    if ((rc = is_empty(filepath, &empty))) return rc;

    if (empty && (rc = emerge_db(filepath))) return rc;

    bool ok = false;
    if ((rc = check_integrity(filepath, &ok))) return rc;
    if (!ok) return error(RC_FAIL, "damaged sched database");

    return rc;
}

enum rc sched_open(char const filepath[DCP_PATH_SIZE])
{
    enum rc rc = RC_DONE;

    if ((rc = xsql_open(filepath, &sqlite3_db))) goto cleanup;
    if ((rc = sched_job_module_init(sqlite3_db))) goto cleanup;
    if ((rc = sched_seq_module_init(sqlite3_db))) goto cleanup;
    if ((rc = sched_prod_module_init(sqlite3_db))) goto cleanup;
    if ((rc = sched_db_module_init(sqlite3_db))) goto cleanup;

    return rc;

cleanup:
    sqlite3_close(sqlite3_db);
    return rc;
}

enum rc sched_close(void)
{
    sched_db_module_del();
    sched_prod_module_del();
    sched_seq_module_del();
    sched_job_module_del();
    return xsql_close(sqlite3_db, false);
}

struct sqlite3 *sched_db(void) { return sqlite3_db; }

enum rc sched_submit_job(struct job *job)
{
    enum rc rc = RC_DONE;
    if ((rc = xsql_begin_transaction(sqlite3_db))) return rc;

    struct sched_job j = SCHED_JOB_INIT(job->db_id, job->multi_hits,
                                        job->hmmer3_compat, (int64_t)utc_now());
    if ((rc = sched_job_add(&j))) goto cleanup;

    job->id = j.id;
    struct cco_iter iter = cco_queue_iter(&job->seqs);
    struct seq *seq = NULL;
    cco_iter_for_each_entry(seq, &iter, node)
    {
        if ((rc = sched_seq_add(j.id, seq->name, seq->str.len, seq->str.data)))
            goto cleanup;
    }

cleanup:
    if (rc) return xsql_rollback_transaction(sqlite3_db);
    return xsql_end_transaction(sqlite3_db);
}

enum rc check_integrity(char const *filepath, bool *ok)
{
    char tmp[] = XFILE_PATH_TEMP_TEMPLATE;
    enum rc rc = RC_DONE;

    if ((rc = create_ground_truth_db(tmp))) return rc;
    if ((rc = sqldiff_compare(filepath, tmp, ok))) goto cleanup;

cleanup:
    remove(tmp);
    return rc;
}

enum rc create_ground_truth_db(char *filepath)
{
    enum rc rc = RC_DONE;
    if ((rc = xfile_mktemp(filepath))) return rc;
    if ((rc = touch_db(filepath))) return rc;
    if ((rc = emerge_db(filepath))) return rc;
    return rc;
}

enum rc emerge_db(char const *filepath)
{
    enum rc rc = RC_DONE;
    struct sqlite3 *db = NULL;
    if ((rc = xsql_open(filepath, &db))) goto cleanup;

    if ((rc = xsql_exec(db, (char const *)sched_schema, 0, 0))) goto cleanup;

    return xsql_close(db, false);

cleanup:
    xsql_close(sqlite3_db, true);
    return rc;
}

static int is_empty_cb(void *empty, int argc, char **argv, char **cols)
{
    *((bool *)empty) = false;
    return 0;
}

enum rc is_empty(char const *filepath, bool *empty)
{
    enum rc rc = RC_DONE;
    struct sqlite3 *db = NULL;
    if ((rc = xsql_open(filepath, &db))) goto cleanup;

    *empty = true;
    static char const *const sql = "SELECT name FROM sqlite_master;";
    if ((rc = xsql_exec(db, sql, is_empty_cb, empty))) goto cleanup;

    return xsql_close(db, false);

cleanup:
    xsql_close(sqlite3_db, true);
    return rc;
}

enum rc touch_db(char const *filepath)
{
    enum rc rc = RC_DONE;
    struct sqlite3 *db = NULL;
    if ((rc = xsql_open(filepath, &db))) goto cleanup;
    return xsql_close(db, false);

cleanup:
    xsql_close(sqlite3_db, true);
    return rc;
}
