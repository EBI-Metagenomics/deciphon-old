#ifndef DCP_HYPOTHESIS_H
#define DCP_HYPOTHESIS_H

#include "imm/imm.h"

struct imm_task;

struct hypothesis
{
    struct imm_task *task;
    struct imm_prod prod;
};

#endif
