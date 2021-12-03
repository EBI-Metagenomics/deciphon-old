#ifndef PRO_NODE_H
#define PRO_NODE_H

#include "imm/imm.h"
#include "nuclt_dist.h"

struct dcp_pro_node
{
    union
    {
        struct imm_frame_state M;
        struct
        {
            struct imm_frame_state state;
            struct dcp_nuclt_dist nucltd;
        } match;
    };
    struct imm_frame_state I;
    struct imm_mute_state D;
};

#endif
