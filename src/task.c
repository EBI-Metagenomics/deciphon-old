#include "task.h"
#include "deciphon/deciphon.h"
#include "results.h"
#include "sequence.h"
#include <stdlib.h>
#include <string.h>

struct dcp_task
{
    struct dcp_task_cfg cfg;
    struct list         sequences;
    struct dcp_results* results;
};

static void free_sequences(struct dcp_task* task);

void dcp_task_add(struct dcp_task* task, char const* sequence)
{
    struct sequence* seq = malloc(sizeof(*seq));
    seq->sequence = strdup(sequence);
    list_init(&seq->link);
    list_add(&task->sequences, &seq->link);
}

struct dcp_task* dcp_task_create(struct dcp_task_cfg cfg)
{
    struct dcp_task* task = malloc(sizeof(*task));
    task->cfg = cfg;
    list_init(&task->sequences);
    task->results = dcp_results_create();
    return task;
}

void dcp_task_destroy(struct dcp_task const* task)
{
    free_sequences((struct dcp_task*)task);
    free((void*)task);
}

void dcp_task_reset(struct dcp_task* task)
{
    free_sequences(task);
    task->results = dcp_results_create();
}

struct dcp_results const* dcp_task_results(struct dcp_task const* task) { return task->results; }

void task_add_result(struct dcp_task* task, struct dcp_result* result) { results_add(task->results, result); }

struct dcp_task_cfg const* task_cfg(struct dcp_task* task) { return &task->cfg; }

struct sequence const* task_first_sequence(struct dcp_task* task)
{
    struct list* i = list_head(&task->sequences);
    return i ? container_of(i, struct sequence, link) : NULL;
}

struct sequence const* task_next_sequence(struct dcp_task* task, struct sequence const* sequence)
{
    struct list* i = list_next(&task->sequences, &sequence->link);
    return i ? container_of(i, struct sequence, link) : NULL;
}

static void free_sequences(struct dcp_task* task)
{
    struct list* i = list_head(&task->sequences);
    while (i) {
        struct list      tmp = *i;
        list_del(i);
        struct sequence* seq = container_of(i, struct sequence, link);
        free((void*)seq->sequence);
        free(seq);
        i = list_next(&task->sequences, &tmp);
    }
    list_init(&task->sequences);
}
