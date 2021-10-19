#ifndef DCP_TARGET_H
#define DCP_TARGET_H

#include "cco/cco.h"
#include "dcp/export.h"

struct imm_seq;

struct dcp_target
{
    struct imm_seq const *seq;
    struct cco_queue queue;
};

#endif
