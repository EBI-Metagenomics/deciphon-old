#include "core/sched_dump.h"
#include "jx.h"
#include "sched_structs.h"

char *sched_dump_db(struct sched_db const *db, char buffer[])
{
    char *p = buffer;
    p += jw_object_open(p);

    p += jw_string(p, "id");
    p += jw_colon(p);
    p += jw_long(p, db->id);

    p += jw_comma(p);

    p += jw_string(p, "xxh3");
    p += jw_colon(p);
    p += jw_long(p, db->xxh3);

    p += jw_comma(p);

    p += jw_string(p, "filename");
    p += jw_colon(p);
    p += jw_string(p, db->filename);

    p += jw_comma(p);

    p += jw_string(p, "hmm_id");
    p += jw_colon(p);
    p += jw_long(p, db->hmm_id);

    p += jw_object_close(p);

    return buffer;
}

char *sched_dump_hmm(struct sched_hmm const *hmm, char buffer[])
{
    char *p = buffer;
    p += jw_object_open(p);

    p += jw_string(p, "id");
    p += jw_colon(p);
    p += jw_long(p, hmm->id);

    p += jw_comma(p);

    p += jw_string(p, "xxh3");
    p += jw_colon(p);
    p += jw_long(p, hmm->xxh3);

    p += jw_comma(p);

    p += jw_string(p, "filename");
    p += jw_colon(p);
    p += jw_string(p, hmm->filename);

    p += jw_comma(p);

    p += jw_string(p, "job_id");
    p += jw_colon(p);
    p += jw_long(p, hmm->job_id);

    p += jw_object_close(p);

    return buffer;
}

char *sched_dump_job(struct sched_job const *job, char buffer[])
{
    char *p = buffer;
    p += jw_object_open(p);

    p += jw_string(p, "id");
    p += jw_colon(p);
    p += jw_long(p, job->id);

    p += jw_comma(p);

    p += jw_string(p, "type");
    p += jw_colon(p);
    p += jw_long(p, job->type);

    p += jw_comma(p);

    p += jw_string(p, "state");
    p += jw_colon(p);
    p += jw_string(p, job->state);

    p += jw_comma(p);

    p += jw_string(p, "progress");
    p += jw_colon(p);
    p += jw_long(p, job->progress);

    p += jw_comma(p);

    p += jw_string(p, "error");
    p += jw_colon(p);
    p += jw_string(p, job->error);

    p += jw_comma(p);

    p += jw_string(p, "submission");
    p += jw_colon(p);
    p += jw_long(p, job->submission);

    p += jw_comma(p);

    p += jw_string(p, "exec_started");
    p += jw_colon(p);
    p += jw_long(p, job->exec_started);

    p += jw_comma(p);

    p += jw_string(p, "exec_ended");
    p += jw_colon(p);
    p += jw_long(p, job->exec_ended);

    p += jw_object_close(p);

    return buffer;
}

char *sched_dump_scan(struct sched_scan const *scan, char buffer[])
{
    char *p = buffer;
    p += jw_object_open(p);

    p += jw_string(p, "id");
    p += jw_colon(p);
    p += jw_long(p, scan->id);

    p += jw_comma(p);

    p += jw_string(p, "db_id");
    p += jw_colon(p);
    p += jw_long(p, scan->db_id);

    p += jw_comma(p);

    p += jw_string(p, "multi_hits");
    p += jw_colon(p);
    p += jw_bool(p, scan->multi_hits);

    p += jw_comma(p);

    p += jw_string(p, "hmmer3_compat");
    p += jw_colon(p);
    p += jw_bool(p, scan->hmmer3_compat);

    p += jw_comma(p);

    p += jw_string(p, "job_id");
    p += jw_colon(p);
    p += jw_long(p, scan->job_id);

    p += jw_object_close(p);

    return buffer;
}
