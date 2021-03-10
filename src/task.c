#include "task.h"
#include "deciphon/deciphon.h"
#include "free.h"
#include "results.h"
#include "sequence.h"
#include <stdlib.h>
#include <string.h>

struct dcp_task
{
    CList               sequences;
    struct dcp_results* results;
};

void dcp_task_add(struct dcp_task* task, char const* sequence)
{
    struct sequence* seq = malloc(sizeof(*seq));
    seq->sequence = strdup(sequence);
    c_list_init(&seq->link);
    c_list_link_tail(&task->sequences, &seq->link);
}

struct dcp_task* dcp_task_create(void)
{
    struct dcp_task* task = malloc(sizeof(*task));
    c_list_init(&task->sequences);
    task->results = dcp_results_create();
    return task;
}

void dcp_task_destroy(struct dcp_task const* task)
{
    dcp_task_reset((struct dcp_task*)task);
    free_c(task);
}

void dcp_task_reset(struct dcp_task* task)
{
    struct sequence* iter = NULL;
    struct sequence* safe = NULL;
    c_list_for_each_entry_safe (iter, safe, &task->sequences, link) {
        c_list_unlink(&iter->link);
        free_c(iter);
    }
    c_list_init(&task->sequences);
    task->results = dcp_results_create();
}

struct dcp_results const* dcp_task_results(struct dcp_task const* task) { return task->results; }

void task_add_result(struct dcp_task* task, struct dcp_result* result) { results_add(task->results, result); }

struct sequence const* task_first_sequence(struct dcp_task* task)
{
    return c_list_first_entry(&task->sequences, struct sequence const, link);
}

struct sequence const* task_next_sequence(struct dcp_task* task, struct sequence const* sequence)
{
    CList const* curr = &sequence->link;
    CList const* next = curr->next;
    if (next == &task->sequences)
        return NULL;
    return c_list_entry(next, struct sequence const, link);
}
