#ifndef TASK_H
#define TASK_H

#include "lib/c-list.h"

struct nmm_profile;

struct task
{
    struct nmm_profile const* profile;
    struct CList              link;
};

struct task* task_create(struct nmm_profile const* prof);
void         task_destroy(struct task const* task);

#endif
