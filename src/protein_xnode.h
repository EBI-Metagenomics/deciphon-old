#ifndef DCP_PROTEIN_XNODE_H
#define DCP_PROTEIN_XNODE_H

#include "imm/imm.h"

struct dcp_protein_xnode_null
{
    struct imm_frame_state R;
};

struct dcp_protein_xnode_alt
{
    struct imm_mute_state S;
    struct imm_frame_state N;
    struct imm_mute_state B;
    struct imm_mute_state E;
    struct imm_frame_state J;
    struct imm_frame_state C;
    struct imm_mute_state T;
};

struct dcp_protein_xnode
{
    struct dcp_protein_xnode_null null;
    struct dcp_protein_xnode_alt alt;
};

#endif
