#ifndef SCHED_PROD_H
#define SCHED_PROD_H

#include "dcp/prod.h"
#include "sched_limits.h"
#include <stdint.h>
#include <stdio.h>

struct pro_match;
struct sqlite3;

enum dcp_rc sched_prod_module_init(struct sqlite3 *db);
enum dcp_rc sched_prod_add(struct dcp_prod *prod);
enum dcp_rc sched_prod_next(int64_t job_id, int64_t *prod_id);
enum dcp_rc sched_prod_get(struct dcp_prod *prod, int64_t prod_id);
void sched_prod_module_del(void);

void sched_prod_set_job_id(struct dcp_prod *, int64_t);
void sched_prod_set_seq_id(struct dcp_prod *, int64_t);
void sched_prod_set_match_id(struct dcp_prod *, int64_t);

void sched_prod_set_prof_name(struct dcp_prod *, char const[SCHED_NAME_SIZE]);
void sched_prod_set_abc_name(struct dcp_prod *, char const[SCHED_SHORT_SIZE]);

void sched_prod_set_loglik(struct dcp_prod *, double);
void sched_prod_set_null_loglik(struct dcp_prod *, double);

void sched_prod_set_model(struct dcp_prod *, char const[SCHED_SHORT_SIZE]);
void sched_prod_set_version(struct dcp_prod *, char const[SCHED_SHORT_SIZE]);

enum dcp_rc sched_prod_write_preamble(struct dcp_prod *prod, FILE *restrict fd);
enum dcp_rc sched_prod_write_match(FILE *restrict fd, struct pro_match const *);
enum dcp_rc sched_prod_write_match_sep(FILE *restrict fd);

enum dcp_rc sched_prod_add_from_tsv(FILE *restrict fd);

#endif
