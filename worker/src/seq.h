#ifndef SEQ_H
#define SEQ_H

#include "cco/cco.h"
#include "common/limits.h"
#include "imm/imm.h"

struct seq
{
    char name[SEQ_NAME_SIZE];
    struct imm_str str;
    struct cco_node node;
};

void seq_init(struct seq *seq, char const *name, struct imm_str str);

#endif
