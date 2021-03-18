#ifndef TASK_H
#define TASK_H

#include "dcp/dcp.h"
#include "list.h"
#include "mpool.h"

struct dcp_results;
struct dcp_task_cfg;
struct sequence;

struct dcp_task
{
    struct dcp_task_cfg cfg;
    struct list         sequences;
    struct llist_list   results;
    bool                finished;
    bool                end;
    struct mpool        pool;
    struct llist_node   link;
};

struct dcp_results*        task_alloc_results(struct dcp_task* task);
struct dcp_task_cfg const* task_cfg(struct dcp_task* task);
void                       task_finish(struct dcp_task* task);
struct sequence const*     task_first_seq(struct dcp_task* task);
struct sequence const*     task_next_seq(struct dcp_task* task, struct sequence const* sequence);
void                       task_push_results(struct dcp_task* task, struct dcp_results* results);

#endif
