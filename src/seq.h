#ifndef SEQ_H
#define SEQ_H

#include "cco/cco.h"
#include "dcp_limits.h"
#include "imm/imm.h"

struct seq
{
    char name[DCP_SEQ_NAME_SIZE];
    struct imm_str str;
    struct cco_node node;
};

void seq_init(struct seq *seq, char const id[static 1], struct imm_str str);

#endif
