#ifndef SEQ_H
#define SEQ_H

#include "cco/cco.h"
#include "imm/imm.h"

struct dcp_seq
{
    struct imm_seq seq;
    struct cco_node node;
};

static inline void dcp_seq_init(struct dcp_seq *seq, struct imm_seq iseq)
{
    seq->seq = iseq;
    cco_node_init(&seq->node);
}

#endif
