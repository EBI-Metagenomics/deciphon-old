#ifndef SCAN_CFG_H
#define SCAN_CFG_H

#include <stdbool.h>

struct scan_cfg
{
    int nthreads;
    double lrt_threshold;
    bool multi_hits;
    bool hmmer3_compat;
};

#endif
