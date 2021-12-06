#ifndef PROD_H
#define PROD_H

#include "dcp_limits.h"
#include <stdint.h>

struct prod
{
    int64_t id;

    int64_t job_id;
    int64_t seq_id;
    int64_t match_id;

    char prof_name[DCP_PROF_NAME_SIZE];
    char abc_name[DCP_ABC_NAME_SIZE];

    double loglik;
    double null_loglik;

    char prof_typeid[DCP_PROFILE_TYPEID_SIZE];
    char version[DCP_VERSION_SIZE];

    struct array *match;
};

double prod_lrt(struct prod const *prod);

#endif
