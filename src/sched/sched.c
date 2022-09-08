#include "deciphon/sched/sched.h"
#include "deciphon/core/logging.h"
#include "deciphon/core/rc.h"
#include "jx.h"
#include "xjson.h"
#include "zc_string_static.h"
#include <string.h>

void sched_db_init(struct sched_db *db) { memset(db, 0, sizeof(*db)); }

void sched_hmm_init(struct sched_hmm *hmm) { memset(hmm, 0, sizeof(*hmm)); }

void sched_job_init(struct sched_job *job) { memset(job, 0, sizeof(*job)); }

void sched_scan_init(struct sched_scan *scan)
{
    memset(scan, 0, sizeof(*scan));
}

void sched_seq_init(struct sched_seq *seq) { memset(seq, 0, sizeof(*seq)); }

enum rc sched_db_parse(struct sched_db *db, struct jr *jr)
{
    db->id = jr_long_of(jr, "id");
    db->xxh3 = jr_long_of(jr, "xxh3");
    zc_strlcpy(db->filename, jr_string_of(jr, "filename"), sizeof db->filename);
    db->hmm_id = jr_long_of(jr, "hmm_id");

    return jr_error() ? einval("failed to parse db") : RC_OK;
}

enum rc sched_hmm_parse(struct sched_hmm *h, struct jr *jr)
{
    h->id = jr_long_of(jr, "id");
    h->xxh3 = jr_long_of(jr, "xxh3");
    zc_strlcpy(h->filename, jr_string_of(jr, "filename"), sizeof h->filename);
    h->job_id = jr_long_of(jr, "job_id");

    return jr_error() ? einval("failed to parse hmm") : RC_OK;
}

enum rc sched_job_parse(struct sched_job *j, struct jr *jr)
{
    j->id = jr_long_of(jr, "id");
    j->type = (int)jr_long_of(jr, "type");
    zc_strlcpy(j->state, jr_string_of(jr, "state"), sizeof j->state);
    j->progress = (int)jr_long_of(jr, "progress");
    zc_strlcpy(j->error, jr_string_of(jr, "error"), sizeof j->error);
    j->submission = jr_long_of(jr, "submission");
    j->exec_started = jr_long_of(jr, "exec_started");
    j->exec_ended = jr_long_of(jr, "exec_ended");

    return jr_error() ? einval(jr->cursor.json) : RC_OK;
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

enum rc sched_seq_parse(struct sched_seq *s, struct jr *jr)
{
    s->id = jr_long_of(jr, "id");
    s->scan_id = jr_long_of(jr, "scan_id");
    zc_strlcpy(s->name, jr_string_of(jr, "name"), sizeof s->name);
    zc_strlcpy(s->data, jr_string_of(jr, "data"), sizeof s->data);

    return jr_error() ? einval("failed to parse seq") : RC_OK;
}
