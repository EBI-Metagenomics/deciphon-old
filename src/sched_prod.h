#ifndef SCHED_PROD_H
#define SCHED_PROD_H

#include "sched_limits.h"
#include <stdint.h>

struct sqlite3;

struct sched_prod
{
    int64_t id;

    int64_t job_id;
    int64_t seq_id;
    int64_t match_id;
    char prof_name[SCHED_NAME_SIZE];

    int64_t start_pos;
    int64_t end_pos;
    char abc_id[SCHED_SHORT_SIZE];
    double loglik;

    double null_loglik;
    char model[SCHED_SHORT_SIZE];
    char version[SCHED_SHORT_SIZE];
    char match_data[SCHED_DATA_SIZE];
};

#define SCHED_PROD_INIT(job_id, seq_id, match_id)                              \
    {                                                                          \
        0, job_id, seq_id, match_id, "", 0, 0, "", 0., 0., "", "", ""          \
    }

enum dcp_rc sched_prod_module_init(struct sqlite3 *db);
enum dcp_rc sched_prod_add(struct sched_prod *prod);
enum dcp_rc sched_prod_get(struct sched_prod *prod, int64_t prod_id);
void sched_prod_module_del(void);

#endif
