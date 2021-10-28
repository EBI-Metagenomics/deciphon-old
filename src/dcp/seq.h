#ifndef DCP_SEQ_H
#define DCP_SEQ_H

#include "cco/cco.h"

struct dcp_seq
{
    char const *data;
    struct cco_node node;
};

static inline void dcp_seq_init(struct dcp_seq *seq, char const *data)
{
    seq->data = data;
    cco_node_init(&seq->node);
}

#endif
