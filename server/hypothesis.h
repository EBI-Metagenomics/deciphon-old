#ifndef HYPOTHESIS_H
#define HYPOTHESIS_H

#include "imm/imm.h"

struct hypothesis
{
    struct imm_task *task;
    struct imm_prod prod;
};

#endif
