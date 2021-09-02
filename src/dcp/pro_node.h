#ifndef DCP_PRO_NODE_H
#define DCP_PRO_NODE_H

#include "imm/imm.h"

struct dcp_pro_node
{
    union
    {
        struct imm_frame_state M;
        struct
        {
            struct imm_frame_state state;
            struct imm_nuclt_lprob nucltp;
            struct imm_codon_marg codonm;
        } match;
    };
    struct imm_frame_state I;
    struct imm_mute_state D;
};

#endif
