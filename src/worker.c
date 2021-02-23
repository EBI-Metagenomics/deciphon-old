#include "worker.h"
#include "deciphon/deciphon.h"
#include "nmm/nmm.h"
#include <stdlib.h>

static int update_thread(void* arg);

struct worker* worker_create(struct dcp_partition* part)
{
    struct worker* worker = malloc(sizeof(*worker));
    worker->partition = part;
    thrd_create(&worker->thread, update_thread, worker);
    return worker;
}

void worker_destroy(struct worker* worker)
{
    IMM_BUG(worker->finished);
    free(worker);
}

static int update_thread(void* arg)
{
    struct worker* restrict worker = arg;
    while (!worker->finished) {

        struct nmm_profile const* prof = NULL;
        while ((prof = dcp_partition_read(worker->partition))) {

            IMM_BUG(nmm_profile_nmodels(prof) != 2);

            struct imm_abc const* abc = nmm_profile_abc(prof);
            struct imm_model*     alt = nmm_profile_get_model(prof, 0);
            struct imm_model*     null = nmm_profile_get_model(prof, 1);
        }
    }
    return 0;
}
