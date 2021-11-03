#ifndef DCP_SEQ_H
#define DCP_SEQ_H

#include "cco/cco.h"
#include "dcp/strlcpy.h"

#define DCP_SEQ_SIZE 16384
#define DCP_SEQ_ID_SIZE 32

struct dcp_seq
{
    char id[DCP_SEQ_ID_SIZE];
    char data[DCP_SEQ_SIZE];
    struct cco_node node;
};

static inline void dcp_seq_init(struct dcp_seq *seq, char const id[static 1],
                                char const data[static 1])
{
    dcp_strlcpy(seq->id, id, DCP_SEQ_ID_SIZE);
    dcp_strlcpy(seq->data, data, DCP_SEQ_SIZE);
    cco_node_init(&seq->node);
}

#endif
