#include "deciphon/deciphon.h"
#include "lib/c11threads.h"
#include "worker.h"
#include <stdlib.h>

void create_master(char const* filepath);

void create_master(char const* filepath)
{
    struct dcp_input* input = dcp_input_create(filepath);

    unsigned               nworkers = 2;
    struct dcp_partition** partitions = malloc(sizeof(*partitions) * nworkers);
    struct worker**        workers = malloc(sizeof(*workers) * nworkers);

    for (unsigned i = 0; i < nworkers; ++i) {
        partitions[i] = dcp_input_create_partition(input, i, nworkers);
        worker_create(partitions[i]);
    }

    for (unsigned i = 0; i < nworkers; ++i) {
        worker_destroy(workers[i]);
        dcp_partition_destroy(partitions[i]);
    }

    free(workers);
    free(partitions);
    dcp_input_destroy(input);
}
