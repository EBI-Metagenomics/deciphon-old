#include "deciphon/server/sched.h"
#include "deciphon/logger.h"
#include "deciphon/rc.h"
#include "xjson.h"
#include <string.h>

void sched_seq_init(struct sched_seq *seq) { memset(seq, 0, sizeof(*seq)); }

void sched_db_init(struct sched_db *db) { memset(db, 0, sizeof(*db)); }

void sched_job_init(struct sched_job *job) { memset(job, 0, sizeof(*job)); }

enum rc sched_db_parse(struct sched_db *db, struct xjson *x, unsigned start)
{
    enum rc rc = RC_OK;

    unsigned nitems = 0;
    for (unsigned i = start; i < x->ntoks && nitems < 3; i += 2)
    {
        if (xjson_eqstr(x, i, "id"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &db->id))) return rc;
        }
        else if (xjson_eqstr(x, i, "xxh3_64"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &db->xxh3_64))) return rc;
        }
        else if (xjson_eqstr(x, i, "filename"))
        {
            if ((rc = xjson_copy_str(x, i + 1, PATH_SIZE, db->filename)))
                return rc;
        }
        else
            return einval("unexpected json key");
        nitems++;
    }

    if (nitems != 3) return einval("expected three items");

    return RC_OK;
}

enum rc sched_job_parse(struct sched_job *job, struct xjson *x, unsigned start)
{
    enum rc rc = RC_OK;

    if (x->ntoks < 2 * 9 + 1) return einval("expected nine items");

    unsigned nitems = 0;
    for (unsigned i = start; i < x->ntoks && nitems < 9; i += 2)
    {
        if (xjson_eqstr(x, i, "id"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &job->id))) return rc;
        }
        else if (xjson_eqstr(x, i, "db_id"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &job->db_id))) return rc;
        }
        else if (xjson_eqstr(x, i, "multi_hits"))
        {
            if (!xjson_is_bool(x, i + 1)) return einval("expected bool");
            job->multi_hits = xjson_to_bool(x, i + 1);
        }
        else if (xjson_eqstr(x, i, "hmmer3_compat"))
        {
            if (!xjson_is_bool(x, i + 1)) return einval("expected bool");
            job->hmmer3_compat = xjson_to_bool(x, i + 1);
        }
        else if (xjson_eqstr(x, i, "state"))
        {
            if ((rc = xjson_copy_str(x, i + 1, JOB_STATE_SIZE, job->state)))
                return rc;
        }
        else if (xjson_eqstr(x, i, "error"))
        {
            if ((rc = xjson_copy_str(x, i + 1, JOB_ERROR_SIZE, job->error)))
                return rc;
        }
        else if (xjson_eqstr(x, i, "submission"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &job->submission))) return rc;
        }
        else if (xjson_eqstr(x, i, "exec_started"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &job->exec_started)))
                return rc;
        }
        else if (xjson_eqstr(x, i, "exec_ended"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &job->exec_ended))) return rc;
        }
        else
            return einval("unexpected json key");
        nitems++;
    }

    if (nitems != 9) return einval("expected three items");

    return RC_OK;
}
