#ifndef DCP_SEQ_H
#define DCP_SEQ_H

#include "cco/cco.h"
#include "dcp/strlcpy.h"

#define DCP_SEQ_SIZE 16384

struct dcp_seq
{
    char data[DCP_SEQ_SIZE];
    struct cco_node node;
};

static inline void dcp_seq_init(struct dcp_seq *seq, char const *data)
{
    dcp_strlcpy(seq->data, data, DCP_SEQ_SIZE);
    cco_node_init(&seq->node);
}

#endif
