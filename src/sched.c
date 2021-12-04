#include "sched.h"
#include "job.h"
#include "logger.h"
#include "macros.h"
#include "path.h"
#include "rc.h"
#include "sched_db.h"
#include "sched_job.h"
#include "sched_macros.h"
#include "sched_prod.h"
#include "sched_schema.h"
#include "sched_seq.h"
#include "sqldiff.h"
#include "utc.h"
#include "xfile.h"
#include <assert.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static struct sqlite3 *sqlite3_db = NULL;

static_assert(SQLITE_VERSION_NUMBER >= 3035000, "We need RETURNING statement");

enum rc check_integrity(char const *filepath, bool *ok);
enum rc create_ground_truth_db(PATH_TEMP_DECLARE(filepath));
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
    if (!ok) return error(FAIL, "damaged sched database");

    return rc;
}

enum rc sched_open(char const filepath[DCP_PATH_SIZE])
{
    enum rc rc = DONE;

    if (sqlite3_open(filepath, &sqlite3_db)) return OPEN_ERROR();
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
    return sqlite3_close(sqlite3_db) ? CLOSE_ERROR() : DONE;
}

struct sqlite3 *sched_db(void) { return sqlite3_db; }

enum rc sched_submit_job(struct job *job)
{
    BEGIN_TRANSACTION_OR_RETURN(sqlite3_db);

    enum rc rc = DONE;

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
    if (rc)
    {
        ROLLBACK_TRANSACTION(sqlite3_db);
        return rc;
    }
    END_TRANSACTION_OR_RETURN(sqlite3_db);
    return rc;
}

enum rc check_integrity(char const *filepath, bool *ok)
{
    PATH_TEMP_DEFINE(tmp);
    enum rc rc = DONE;

    if ((rc = create_ground_truth_db(tmp))) return rc;
    if ((rc = sqldiff_compare(filepath, tmp, ok))) goto cleanup;

cleanup:
    remove(tmp);
    return rc;
}

enum rc create_ground_truth_db(PATH_TEMP_DECLARE(filepath))
{
    enum rc rc = DONE;
    if ((rc = xfile_mktemp(filepath))) return rc;
    if ((rc = touch_db(filepath))) return rc;
    if ((rc = emerge_db(filepath))) return rc;
    return rc;
}

enum rc emerge_db(char const *filepath)
{
    struct sqlite3 *db = NULL;
    if (sqlite3_open(filepath, &db)) return OPEN_ERROR();

    if (sqlite3_exec(db, (char const *)sched_schema, 0, 0, 0))
    {
        enum rc rc = EXEC_ERROR();
        sqlite3_close(db);
        return rc;
    }
    return sqlite3_close(db) ? CLOSE_ERROR() : DONE;
}

static int is_empty_cb(void *empty, int argc, char **argv, char **cols)
{
    *((bool *)empty) = false;
    return 0;
}

enum rc is_empty(char const *filepath, bool *empty)
{
    struct sqlite3 *db = NULL;
    if (sqlite3_open(filepath, &db)) return OPEN_ERROR();

    *empty = true;
    static char const *const sql = "SELECT name FROM sqlite_master;";
    if (sqlite3_exec(db, sql, is_empty_cb, empty, 0))
    {
        enum rc rc = EXEC_ERROR();
        sqlite3_close(db);
        return rc;
    }

    return sqlite3_close(db) ? CLOSE_ERROR() : DONE;
}

enum rc touch_db(char const *filepath)
{
    struct sqlite3 *db = NULL;
    if (sqlite3_open(filepath, &db)) return OPEN_ERROR();
    return sqlite3_close(db) ? CLOSE_ERROR() : DONE;
}
