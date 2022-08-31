#include "deciphon/sched/dump.h"
#include "deciphon/sched/sched.h"
#include "lij.h"

char *sched_dump_hmm(struct sched_hmm const *hmm, char buffer[])
{
    char *p = buffer;
    p += lij_pack_object_open(p);

    p += lij_pack_cstr(p, "id");
    p += lij_pack_colon(p);
    p += lij_pack_int(p, hmm->id);

    p += lij_pack_comma(p);

    p += lij_pack_cstr(p, "xxh3");
    p += lij_pack_colon(p);
    p += lij_pack_int(p, hmm->xxh3);

    p += lij_pack_comma(p);

    p += lij_pack_cstr(p, "filename");
    p += lij_pack_colon(p);
    p += lij_pack_cstr(p, hmm->filename);

    p += lij_pack_comma(p);

    p += lij_pack_cstr(p, "job_id");
    p += lij_pack_colon(p);
    p += lij_pack_int(p, hmm->job_id);

    p += lij_pack_object_close(p);

    return buffer;
}

char *sched_dump_db(struct sched_db const *db, char buffer[])
{
    char *p = buffer;
    p += lij_pack_object_open(p);

    p += lij_pack_cstr(p, "id");
    p += lij_pack_colon(p);
    p += lij_pack_int(p, db->id);

    p += lij_pack_comma(p);

    p += lij_pack_cstr(p, "xxh3");
    p += lij_pack_colon(p);
    p += lij_pack_int(p, db->xxh3);

    p += lij_pack_comma(p);

    p += lij_pack_cstr(p, "filename");
    p += lij_pack_colon(p);
    p += lij_pack_cstr(p, db->filename);

    p += lij_pack_comma(p);

    p += lij_pack_cstr(p, "hmm_id");
    p += lij_pack_colon(p);
    p += lij_pack_int(p, db->hmm_id);

    p += lij_pack_object_close(p);

    return buffer;
}

char *sched_dump_job(struct sched_job const *job, char buffer[])
{
    char *p = buffer;
    p += lij_pack_object_open(p);

    p += lij_pack_cstr(p, "id");
    p += lij_pack_colon(p);
    p += lij_pack_int(p, job->id);

    p += lij_pack_comma(p);

    p += lij_pack_cstr(p, "type");
    p += lij_pack_colon(p);
    p += lij_pack_int(p, job->type);

    p += lij_pack_comma(p);

    p += lij_pack_cstr(p, "state");
    p += lij_pack_colon(p);
    p += lij_pack_cstr(p, job->state);

    p += lij_pack_comma(p);

    p += lij_pack_cstr(p, "progress");
    p += lij_pack_colon(p);
    p += lij_pack_int(p, job->progress);

    p += lij_pack_comma(p);

    p += lij_pack_cstr(p, "error");
    p += lij_pack_colon(p);
    p += lij_pack_cstr(p, job->error);

    p += lij_pack_comma(p);

    p += lij_pack_cstr(p, "submission");
    p += lij_pack_colon(p);
    p += lij_pack_int(p, job->submission);

    p += lij_pack_comma(p);

    p += lij_pack_cstr(p, "exec_started");
    p += lij_pack_colon(p);
    p += lij_pack_int(p, job->exec_started);

    p += lij_pack_comma(p);

    p += lij_pack_cstr(p, "exec_ended");
    p += lij_pack_colon(p);
    p += lij_pack_int(p, job->exec_ended);

    p += lij_pack_object_close(p);

    return buffer;
}
