#include "deciphon/server/sched.h"
#include "deciphon/logger.h"
#include "deciphon/rc.h"
#include "xjson.h"
#include <string.h>

void sched_db_init(struct sched_db *db) { memset(db, 0, sizeof(*db)); }

void sched_hmm_init(struct sched_hmm *hmm) { memset(hmm, 0, sizeof(*hmm)); }

void sched_job_init(struct sched_job *job) { memset(job, 0, sizeof(*job)); }

void sched_scan_init(struct sched_scan *scan)
{
    memset(scan, 0, sizeof(*scan));
}

void sched_seq_init(struct sched_seq *seq) { memset(seq, 0, sizeof(*seq)); }

enum rc sched_db_parse(struct sched_db *db, struct xjson *x, unsigned start)
{
    enum rc rc = RC_OK;
    static unsigned expected_items = 4;

    unsigned nitems = 0;
    for (unsigned i = start; i < x->ntoks && nitems < expected_items; i += 2)
    {
        if (xjson_eqstr(x, i, "id"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &db->id))) return rc;
        }
        else if (xjson_eqstr(x, i, "xxh3"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &db->xxh3))) return rc;
        }
        else if (xjson_eqstr(x, i, "filename"))
        {
            if ((rc = xjson_copy_str(x, i + 1, PATH_SIZE, db->filename)))
                return rc;
        }
        else if (xjson_eqstr(x, i, "hmm_id"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &db->hmm_id))) return rc;
        }
        else
            return einval("unexpected json key");
        nitems++;
    }

    if (nitems != expected_items) return einval("expected four items");

    return RC_OK;
}

enum rc sched_hmm_parse(struct sched_hmm *hmm, struct xjson *x, unsigned start)
{
    enum rc rc = RC_OK;
    static unsigned expected_items = 4;

    unsigned nitems = 0;
    for (unsigned i = start; i < x->ntoks && nitems < expected_items; i += 2)
    {
        if (xjson_eqstr(x, i, "id"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &hmm->id))) return rc;
        }
        else if (xjson_eqstr(x, i, "xxh3"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &hmm->xxh3))) return rc;
        }
        else if (xjson_eqstr(x, i, "filename"))
        {
            if ((rc = xjson_copy_str(x, i + 1, PATH_SIZE, hmm->filename)))
                return rc;
        }
        else if (xjson_eqstr(x, i, "job_id"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &hmm->job_id))) return rc;
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
    static unsigned expected_items = 8;

    unsigned nitems = 0;
    for (unsigned i = start; i < x->ntoks && nitems < expected_items; i += 2)
    {
        if (xjson_eqstr(x, i, "id"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &job->id))) return rc;
        }
        else if (xjson_eqstr(x, i, "type"))
        {
            if ((rc = xjson_bind_int(x, i + 1, &job->type))) return rc;
        }
        else if (xjson_eqstr(x, i, "state"))
        {
            if ((rc = xjson_copy_str(x, i + 1, JOB_STATE_SIZE, job->state)))
                return rc;
        }
        else if (xjson_eqstr(x, i, "progress"))
        {
            if ((rc = xjson_bind_int(x, i + 1, &job->progress))) return rc;
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

    if (nitems != expected_items) return einval("expected eight items");

    return RC_OK;
}

enum rc sched_scan_parse(struct sched_scan *scan, struct xjson *x,
                         unsigned start)
{
    enum rc rc = RC_OK;
    static unsigned expected_items = 5;

    unsigned nitems = 0;
    for (unsigned i = start; i < x->ntoks && nitems < expected_items; i += 2)
    {
        if (xjson_eqstr(x, i, "id"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &scan->id))) return rc;
        }
        else if (xjson_eqstr(x, i, "db_id"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &scan->db_id))) return rc;
        }
        else if (xjson_eqstr(x, i, "multi_hits"))
        {
            if (!xjson_is_bool(x, i + 1)) return einval("expected bool");
            scan->multi_hits = xjson_to_bool(x, i + 1);
        }
        else if (xjson_eqstr(x, i, "hmmer3_compat"))
        {
            if (!xjson_is_bool(x, i + 1)) return einval("expected bool");
            scan->hmmer3_compat = xjson_to_bool(x, i + 1);
        }
        else if (xjson_eqstr(x, i, "job_id"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &scan->job_id))) return rc;
        }
        else
            return einval("unexpected json key");
        nitems++;
    }

    if (nitems != expected_items) return einval("expected five items");

    return RC_OK;
}

enum rc sched_seq_parse(struct sched_seq *seq, struct xjson *x, unsigned start)
{
    enum rc rc = RC_OK;
    static unsigned expected_items = 4;

    unsigned nitems = 0;
    for (unsigned i = start; i < x->ntoks && nitems < expected_items; i += 2)
    {
        if (xjson_eqstr(x, i, "id"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &seq->id))) return rc;
        }
        else if (xjson_eqstr(x, i, "scan_id"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &seq->scan_id))) return rc;
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
