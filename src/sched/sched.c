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
    jr_strcpy_of(jr, "filename", db->filename, sizeof db->filename);
    db->hmm_id = jr_long_of(jr, "hmm_id");

    return jr_error() ? einval("failed to parse db") : RC_OK;
}

enum rc sched_hmm_parse(struct sched_hmm *h, struct jr *jr)
{
    h->id = jr_long_of(jr, "id");
    h->xxh3 = jr_long_of(jr, "xxh3");
    jr_strcpy_of(jr, "filename", h->filename, sizeof h->filename);
    h->job_id = jr_long_of(jr, "job_id");

    return jr_error() ? einval("failed to parse hmm") : RC_OK;
}

enum rc sched_job_parse(struct sched_job *j, struct jr *jr)
{
    j->id = jr_long_of(jr, "id");
    j->type = (int)jr_long_of(jr, "type");
    jr_strcpy_of(jr, "state", j->state, sizeof j->state);
    j->progress = (int)jr_long_of(jr, "progress");
    jr_strcpy_of(jr, "error", j->error, sizeof j->error);
    j->submission = jr_long_of(jr, "submission");
    j->exec_started = jr_long_of(jr, "exec_started");
    j->exec_ended = jr_long_of(jr, "exec_ended");

    return jr_error() ? einval(jr->cursor.json) : RC_OK;
}

enum rc sched_scan_parse(struct sched_scan *s, struct jr *jr)
{
    s->id = jr_long_of(jr, "id");
    s->db_id = jr_long_of(jr, "db_id");
    s->multi_hits = jr_bool_of(jr, "multi_hits");
    s->hmmer3_compat = jr_bool_of(jr, "hmmer3_compat");
    s->job_id = jr_bool_of(jr, "job_id");

    return jr_error() ? einval("failed to parse seq") : RC_OK;
}

enum rc sched_seq_parse(struct sched_seq *s, struct jr *jr)
{
    s->id = jr_long_of(jr, "id");
    s->scan_id = jr_long_of(jr, "scan_id");
    jr_strcpy_of(jr, "name", s->name, sizeof s->name);
    jr_strcpy_of(jr, "data", s->data, sizeof s->data);

    return jr_error() ? einval("failed to parse seq") : RC_OK;
}
