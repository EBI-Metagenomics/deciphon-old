#ifndef DCP_PROD_H
#define DCP_PROD_H

#include "imm/imm.h"
#include "limits.h"
#include <stdint.h>

struct dcp_prod
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

    char match_data[DCP_MATCH_DATA_SIZE];
};

IMM_API double dcp_prod_lrt(struct dcp_prod const *prod);

#endif
