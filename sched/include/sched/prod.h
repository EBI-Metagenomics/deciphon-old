#ifndef SCHED_PROD_H
#define SCHED_PROD_H

#include "common/export.h"
#include "common/limits.h"
#include "common/rc.h"
#include <stdint.h>
#include <stdio.h>

struct sched_prod
{
    int64_t id;

    int64_t job_id;
    int64_t seq_id;

    char profile_name[PROFILE_NAME_SIZE];
    char abc_name[ABC_NAME_SIZE];

    double alt_loglik;
    double null_loglik;

    char profile_typeid[PROFILE_TYPEID_SIZE];
    char version[VERSION_SIZE];

    char match[MATCH_SIZE];
};

typedef void(sched_prod_set_cb)(struct sched_prod *prod, void *arg);

typedef int(sched_prod_write_match_cb)(FILE *fp, void const *match);

EXPORT void sched_prod_init(struct sched_prod *prod, int64_t job_id);
EXPORT enum rc sched_prod_next(struct sched_prod *prod);

EXPORT enum rc sched_prod_write_begin(struct sched_prod const *prod,
                                      unsigned thread_num);
EXPORT enum rc sched_prod_write_match(sched_prod_write_match_cb *cb,
                                      void const *match, unsigned thread_num);
EXPORT enum rc sched_prod_write_match_sep(unsigned thread_num);
EXPORT enum rc sched_prod_write_end(unsigned thread_num);

#endif
