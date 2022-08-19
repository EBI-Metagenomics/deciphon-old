#include "deciphon/sched/dump.h"
#include "deciphon/core/ljson.h"
#include "deciphon/sched/sched.h"

char *sched_dump_hmm(struct sched_hmm const *hmm, unsigned size, char *buffer)
{
    struct ljson_ctx ctx = {0};
    ljson_open(&ctx, size, buffer);
    ljson_int(&ctx, "id", hmm->id);
    ljson_int(&ctx, "xxh3", hmm->xxh3);
    ljson_str(&ctx, "filename", hmm->filename);
    ljson_int(&ctx, "job_id", hmm->job_id);
    ljson_close(&ctx);
    return buffer;
}
