#ifndef DCP_SEQ_H
#define DCP_SEQ_H

#include "cco/cco.h"
#include "dcp/export.h"
#include "dcp/limits.h"
#include "imm/imm.h"

struct dcp_seq
{
    char name[DCP_SEQ_NAME_SIZE];
    struct imm_str str;
    struct cco_node node;
};

void dcp_seq_init(struct dcp_seq *seq, char const id[static 1],
                  struct imm_str str);

#endif
