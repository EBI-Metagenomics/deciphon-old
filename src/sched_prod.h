#ifndef SCHED_PROD_H
#define SCHED_PROD_H

#include "sched_limits.h"
#include <stdint.h>
#include <stdio.h>

struct pro_match;
struct sqlite3;

struct sched_prod
{
    int64_t id;

    int64_t job_id;
    int64_t seq_id;
    int64_t match_id;

    char prof_name[SCHED_NAME_SIZE];
    char abc_name[SCHED_SHORT_SIZE];

    int64_t start_pos;
    int64_t end_pos;

    double loglik;
    double null_loglik;

    char model[SCHED_SHORT_SIZE];
    char version[SCHED_SHORT_SIZE];

    char match_data[SCHED_DATA_SIZE];
};

enum dcp_rc sched_prod_module_init(struct sqlite3 *db);
enum dcp_rc sched_prod_add(struct sched_prod *prod);
enum dcp_rc sched_prod_get(struct sched_prod *prod, int64_t prod_id);
void sched_prod_module_del(void);

void sched_prod_set_job_id(struct sched_prod *, int64_t);
void sched_prod_set_seq_id(struct sched_prod *, int64_t);
void sched_prod_set_match_id(struct sched_prod *, int64_t);

void sched_prod_set_prof_name(struct sched_prod *, char const[SCHED_NAME_SIZE]);
void sched_prod_set_abc_name(struct sched_prod *, char const[SCHED_SHORT_SIZE]);

void sched_prod_set_start_pos(struct sched_prod *, int64_t);
void sched_prod_set_end_pos(struct sched_prod *, int64_t);

void sched_prod_set_loglik(struct sched_prod *, double);
void sched_prod_set_null_loglik(struct sched_prod *, double);

void sched_prod_set_model(struct sched_prod *, char const[SCHED_SHORT_SIZE]);
void sched_prod_set_version(struct sched_prod *, char const[SCHED_SHORT_SIZE]);

enum dcp_rc sched_prod_write_preamble(struct sched_prod *prod,
                                      FILE *restrict fd);
enum dcp_rc sched_prod_write_match(FILE *restrict fd, struct pro_match const *);
enum dcp_rc sched_prod_write_match_sep(FILE *restrict fd);

enum dcp_rc sched_prod_add_from_tsv(FILE *restrict fd);

#endif
