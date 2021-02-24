#include "task.h"
#include "free.h"
#include <stdlib.h>

struct task* task_create(struct nmm_profile const* prof)
{
    struct task* task = malloc(sizeof(*task));
    task->profile = prof;
    c_list_init(&task->link);
    return task;
}

void task_destroy(struct task const* task) { free_c(task); }
