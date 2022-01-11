#include "sched/sched.h"
#include "stmt.h"
#include "common/rc.h"
#include "common/compiler.h"
#include "db.h"
#include "job.h"
#include "prod.h"
#include "common/safe.h"
#include "schema.h"
#include "seq.h"
#include "seq_queue.h"
#include "common/utc.h"
#include "common/xfile.h"
#include "xsql.h"
#include <assert.h>
#include <limits.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

struct sqlite3 *sched = NULL;
char sched_filepath[SCHED_PATH_SIZE] = {0};

#define MIN_SQLITE_VERSION 3031001

static_assert(SQLITE_VERSION_NUMBER >= MIN_SQLITE_VERSION,
              "Minimum sqlite requirement.");

enum rc emerge_db(char const *filepath);
enum rc is_empty(char const *filepath, bool *empty);
enum rc touch_db(char const *filepath);

enum rc sched_setup(char const *filepath)
{
    safe_strcpy(sched_filepath, filepath, ARRAY_SIZE(sched_filepath));

    int thread_safe = sqlite3_threadsafe();
    if (thread_safe == 0) return RC_FAIL;
    if (sqlite3_libversion_number() < MIN_SQLITE_VERSION) return RC_FAIL;

    if (touch_db(filepath)) return failed_to(RC_FAIL, "touch db");

    bool empty = false;
    if (is_empty(filepath, &empty)) return RC_FAIL;

    if (empty && emerge_db(filepath)) return RC_FAIL;

    return RC_DONE;
}

enum rc sched_open(void)
{
    if (xsql_open(sched_filepath, &sched)) goto cleanup;
    if (stmt_init()) goto cleanup;

    return RC_DONE;

cleanup:
    xsql_close(sched);
    return RC_FAIL;
}

enum rc sched_close(void)
{
    stmt_del();
    return xsql_close(sched);
}

enum rc sched_set_job_fail(int64_t job_id, char const *msg)
{
    return job_set_error(job_id, msg, utc_now());
}
enum rc sched_set_job_done(int64_t job_id)
{
    return job_set_done(job_id, utc_now());
}

enum rc sched_add_db(char const *filepath, int64_t *id)
{
    char resolved[PATH_MAX] = {0};
    char *ptr = realpath(filepath, resolved);
    if (!ptr) return RC_FAIL;

    struct db db = {0};
    int rc = db_has(resolved, &db);
    if (rc == RC_DONE)
    {
        *id = db.id;
        if (strcmp(db.filepath, resolved) == 0) return RC_DONE;
        return RC_FAIL;
    }

    if (rc == RC_NOTFOUND) return db_add(resolved, id);
    return RC_FAIL;
}

enum rc sched_cpy_db_filepath(unsigned size, char *filepath, int64_t id)
{
    struct db db = {0};
    int code = db_get_by_id(&db, id);
    if (code == RC_NOTFOUND) return RC_NOTFOUND;
    if (code != RC_DONE) return RC_FAIL;
    safe_strcpy(filepath, db.filepath, size);
    return RC_DONE;
}

enum rc sched_get_job(struct sched_job *job) { return job_get(job); }

enum rc sched_begin_job_submission(struct sched_job *job)
{
    if (xsql_begin_transaction(sched)) return RC_FAIL;
    seq_queue_init();
    return RC_DONE;
}

void sched_add_seq(struct sched_job *job, char const *name, char const *data)
{
    seq_queue_add(job->id, name, data);
}

enum rc sched_rollback_job_submission(struct sched_job *job)
{
    return xsql_rollback_transaction(sched);
}

enum rc sched_end_job_submission(struct sched_job *job)
{
    if (job_submit(job)) return RC_FAIL;

    for (unsigned i = 0; i < seq_queue_size(); ++i)
    {
        struct sched_seq *seq = seq_queue_get(i);
        seq->job_id = job->id;
        if (seq_submit(seq)) return xsql_rollback_transaction(sched);
    }

    return xsql_end_transaction(sched);
}

enum rc sched_begin_prod_submission(unsigned num_threads)
{
    assert(num_threads > 0);
    if (prod_begin_submission(num_threads)) return RC_FAIL;
    return RC_DONE;
}

enum rc sched_end_prod_submission(void)
{
    if (prod_end_submission()) return RC_FAIL;
    return RC_DONE;
}

enum rc sched_next_pending_job(struct sched_job *job)
{
    return job_next_pending(job);
}

enum rc emerge_db(char const *filepath)
{
    int rc = 0;
    struct sqlite3 *db = NULL;
    if ((rc = xsql_open(filepath, &db))) goto cleanup;

    if ((rc = xsql_exec(db, (char const *)schema, 0, 0))) goto cleanup;

    return xsql_close(db);

cleanup:
    xsql_close(sched);
    return rc;
}

static int is_empty_cb(void *empty, int argc, char **argv, char **cols)
{
    *((bool *)empty) = false;
    return 0;
}

enum rc is_empty(char const *filepath, bool *empty)
{
    int rc = 0;
    struct sqlite3 *db = NULL;
    if ((rc = xsql_open(filepath, &db))) goto cleanup;

    *empty = true;
    static char const *const sql = "SELECT name FROM sqlite_master;";
    if ((rc = xsql_exec(db, sql, is_empty_cb, empty))) goto cleanup;

    return xsql_close(db);

cleanup:
    xsql_close(sched);
    return rc;
}

enum rc touch_db(char const *filepath)
{
    struct sqlite3 *db = NULL;
    if (xsql_open(filepath, &db)) goto cleanup;
    return xsql_close(db);

cleanup:
    xsql_close(sched);
    return RC_FAIL;
}
