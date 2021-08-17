#ifndef DCP_PRO_SPECIAL_NODE_H
#define DCP_PRO_SPECIAL_NODE_H

#include "imm/imm.h"

struct dcp_pro_special_node
{
    struct
    {
        struct imm_frame_state R;
    } null;

    struct
    {
        struct imm_mute_state S;
        struct imm_frame_state N;
        struct imm_mute_state B;
        struct imm_mute_state E;
        struct imm_frame_state J;
        struct imm_frame_state C;
        struct imm_mute_state T;
    } alt;
};

#endif
