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
    static unsigned expected_items = 3;

    unsigned nitems = 0;
    for (unsigned i = start; i < x->ntoks && nitems < expected_items; i += 2)
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

    if (nitems != expected_items) return einval("expected three items");

    return RC_OK;
}

enum rc sched_job_parse(struct sched_job *job, struct xjson *x, unsigned start)
{
    enum rc rc = RC_OK;
    static unsigned expected_items = 9;

    if (x->ntoks < 2 * 9 + 1) return einval("expected nine items");

    unsigned nitems = 0;
    for (unsigned i = start; i < x->ntoks && nitems < expected_items; i += 2)
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

    if (nitems != expected_items) return einval("expected nine items");

    return RC_OK;
}

enum rc sched_seq_parse(struct sched_seq *seq, struct xjson *x, unsigned start)
{
    enum rc rc = RC_OK;
    static unsigned expected_items = 4;

    if (x->ntoks < 2 * expected_items + 1) return einval("expected four items");

    unsigned nitems = 0;
    for (unsigned i = start; i < x->ntoks && nitems < expected_items; i += 2)
    {
        if (xjson_eqstr(x, i, "id"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &seq->id))) return rc;
        }
        else if (xjson_eqstr(x, i, "job_id"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &seq->job_id))) return rc;
        }
        else if (xjson_eqstr(x, i, "name"))
        {
            if ((rc = xjson_copy_str(x, i + 1, SEQ_NAME_SIZE, seq->name)))
                return rc;
        }
        else if (xjson_eqstr(x, i, "data"))
        {
            if ((rc = xjson_copy_str(x, i + 1, SEQ_SIZE, seq->data))) return rc;
        }
        else
            return einval("unexpected json key");
        nitems++;
    }

    if (nitems != expected_items) return einval("expected four items");

    return RC_OK;
}

static char const rc_name[][9] = {"ok",  "end",    "notfound", "efail",
                                  "eio", "einval", "enomem",   "eparse"};

enum rc sched_rc_resolve(unsigned len, char const *str, enum sched_rc *rc)
{
    for (unsigned i = 0; i < ARRAY_SIZE(rc_name); ++i)
    {
        if (!strncmp(rc_name[i], str, len))
        {
            *rc = (enum sched_rc)i;
            return RC_OK;
        }
    }
    return einval("invalid return code");
}
