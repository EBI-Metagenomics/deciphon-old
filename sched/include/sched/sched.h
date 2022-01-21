#ifndef SCHED_SCHED_H
#define SCHED_SCHED_H

#include "common/export.h"
#include "common/limits.h"
#include "common/rc.h"
#include "sched/db.h"
#include "sched/job.h"
#include "sched/prod.h"
#include "sched/seq.h"

EXPORT enum rc sched_setup(char const *filepath);
EXPORT enum rc sched_open(void);
EXPORT enum rc sched_close(void);

#endif
