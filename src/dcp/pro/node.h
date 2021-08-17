#ifndef DCP_PRO_NODE_H
#define DCP_PRO_NODE_H

#include "imm/imm.h"

struct dcp_pro_node
{
    struct imm_frame_state M;
    struct imm_frame_state I;
    struct imm_mute_state D;
    struct imm_nuclt_lprob nucltp;
    struct imm_codon_marg codonm;
};

#endif
