#include "sched/sched.h"
#include "common/compiler.h"
#include "common/rc.h"
#include "common/safe.h"
#include "common/utc.h"
#include "common/xfile.h"
#include "db.h"
#include "job.h"
#include "prod.h"
#include "schema.h"
#include "seq.h"
#include "seq_queue.h"
#include "stmt.h"
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
    if (thread_safe == 0) return efail("not thread safe");
    if (sqlite3_libversion_number() < MIN_SQLITE_VERSION)
        return efail("old sqlite3");

    if (touch_db(filepath)) return efail("touch db");

    bool empty = false;
    if (is_empty(filepath, &empty)) BUG();

    if (empty && emerge_db(filepath)) return efail("emerge db");

    return DONE;
}

enum rc sched_open(void)
{
    if (xsql_open(sched_filepath, &sched)) goto cleanup;
    if (stmt_init()) goto cleanup;

    return DONE;

cleanup:
    xsql_close(sched);
    return EFAIL;
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
    if (!ptr) return efail("realpath");

    struct db db = {0};
    int rc = db_has(resolved, &db);
    if (rc == DONE)
    {
        *id = db.id;
        if (strcmp(db.filepath, resolved) == 0) return DONE;
        return efail("copy resolved filepath");
    }

    if (rc == NOTFOUND) return db_add(resolved, id);
    return EFAIL;
}

enum rc sched_cpy_db_filepath(unsigned size, char *filepath, int64_t id)
{
    struct db db = {0};
    int code = db_get_by_id(&db, id);
    if (code == NOTFOUND) return NOTFOUND;
    if (code != DONE) return EFAIL;
    safe_strcpy(filepath, db.filepath, size);
    return DONE;
}

enum rc sched_get_job(struct sched_job *job) { return job_get(job); }

enum rc sched_begin_job_submission(struct sched_job *job)
{
    if (xsql_begin_transaction(sched)) return EFAIL;
    seq_queue_init();
    return DONE;
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
    if (job_submit(job)) return efail("submit job");

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
    if (prod_begin_submission(num_threads)) return EFAIL;
    return DONE;
}

enum rc sched_end_prod_submission(void)
{
    if (prod_end_submission()) return EFAIL;
    return DONE;
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
    return efail("touch db");
}
