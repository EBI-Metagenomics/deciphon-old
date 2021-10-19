#ifndef DCP_TARGET_H
#define DCP_TARGET_H

#include "cco/cco.h"
#include "dcp/export.h"
#include "imm/imm.h"

struct dcp_target
{
    struct imm_seq seq;
    struct cco_node node;
};

static inline void dcp_target_init(struct dcp_target *tgt, struct imm_seq seq)
{
    tgt->seq = seq;
    cco_node_init(&tgt->node);
}

#endif
