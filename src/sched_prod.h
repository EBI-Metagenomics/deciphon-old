#ifndef SCHED_PROD_H
#define SCHED_PROD_H

#include "dcp/prod.h"
#include <stdint.h>
#include <stdio.h>

struct array;
struct pro_match;
struct sqlite3;
struct tok;

struct sched_prod
{
    int64_t id;

    int64_t job_id;
    int64_t seq_id;
    int64_t match_id;

    char prof_name[DCP_PROF_NAME_SIZE];
    char abc_name[DCP_ABC_NAME_SIZE];

    double loglik;
    double null_loglik;

    char model[DCP_MODEL_SIZE];
    char version[DCP_VERSION_SIZE];

    struct array *match;
};

enum dcp_rc sched_prod_module_init(struct sqlite3 *db);
enum dcp_rc sched_prod_add(struct sched_prod *prod);
enum dcp_rc sched_prod_next(int64_t job_id, int64_t *prod_id);
enum dcp_rc sched_prod_get(struct sched_prod *prod, int64_t prod_id);
void sched_prod_module_del(void);

void sched_prod_set_job_id(struct sched_prod *, int64_t);
void sched_prod_set_seq_id(struct sched_prod *, int64_t);
void sched_prod_set_match_id(struct sched_prod *, int64_t);

void sched_prod_set_prof_name(struct sched_prod *,
                              char const[DCP_PROF_NAME_SIZE]);
void sched_prod_set_abc_name(struct sched_prod *, char const *);

void sched_prod_set_loglik(struct sched_prod *, double);
void sched_prod_set_null_loglik(struct sched_prod *, double);

void sched_prod_set_model(struct sched_prod *, char const *);
void sched_prod_set_version(struct sched_prod *, char const *);

enum dcp_rc sched_prod_write_preamble(struct sched_prod *prod,
                                      FILE *restrict fd);
enum dcp_rc sched_prod_write_match(FILE *restrict fd, struct pro_match const *);
enum dcp_rc sched_prod_write_match_sep(FILE *restrict fd);
enum dcp_rc sched_prod_write_nl(FILE *restrict fd);

enum dcp_rc sched_prod_add_from_tsv(FILE *restrict fd, struct tok *tok);

#endif
