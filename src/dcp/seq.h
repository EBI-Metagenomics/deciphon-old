#ifndef DCP_SEQ_H
#define DCP_SEQ_H

#include "cco/cco.h"
#include "dcp/export.h"

#define DCP_SEQ_SIZE 16384
#define DCP_SEQ_ID_SIZE 32

struct dcp_seq
{
    char id[DCP_SEQ_ID_SIZE];
    char data[DCP_SEQ_SIZE];
    struct cco_node node;
};

void dcp_seq_init(struct dcp_seq *seq, char const id[static 1],
                                char const data[static 1]);

#endif
