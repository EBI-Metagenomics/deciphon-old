#ifndef DECIPHON_MODEL_PROTEIN_NODE_H
#define DECIPHON_MODEL_PROTEIN_NODE_H

#include "deciphon/model/nuclt_dist.h"
#include "imm/imm.h"

struct protein_node
{
    union
    {
        struct imm_frame_state M;
        struct
        {
            struct imm_frame_state state;
            struct nuclt_dist nucltd;
        } match;
    };
    struct imm_frame_state I;
    struct imm_mute_state D;
};

#endif
