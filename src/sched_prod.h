#ifndef SCHED_PROD_H
#define SCHED_PROD_H

#include "prod.h"
#include <stdint.h>
#include <stdio.h>

struct protein_match;

enum rc sched_prod_module_init(void);
enum rc sched_prod_add(struct prod *prod);
enum rc sched_prod_next(int64_t job_id, int64_t *prod_id);
enum rc sched_prod_get(struct prod *prod, int64_t prod_id);
void sched_prod_module_del(void);

void sched_prod_set_job_id(struct prod *, int64_t);
void sched_prod_set_seq_id(struct prod *, int64_t);
void sched_prod_set_match_id(struct prod *, int64_t);

void sched_prod_set_prof_name(struct prod *, char const[DCP_PROF_NAME_SIZE]);
void sched_prod_set_abc_name(struct prod *, char const *);

void sched_prod_set_loglik(struct prod *, double);
void sched_prod_set_null_loglik(struct prod *, double);

void sched_prod_set_prof_typeid(struct prod *, char const *);
void sched_prod_set_version(struct prod *, char const *);

enum rc sched_prod_write_preamble(struct prod *prod, FILE *restrict fd);
enum rc sched_prod_write_match(FILE *restrict fd, struct protein_match const *);
enum rc sched_prod_write_match_sep(FILE *restrict fd);
enum rc sched_prod_write_nl(FILE *restrict fd);

enum rc sched_prod_add_from_tsv(FILE *restrict fd);

#endif
