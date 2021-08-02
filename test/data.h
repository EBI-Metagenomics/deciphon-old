#ifndef TEST_DATA_H
#define TEST_DATA_H

#include "dcp/dcp.h"
#include "imm/imm.h"

struct pro_profile_3core_nodes
{
    struct dcp_pro_cfg cfg;
    unsigned core_size;
    imm_float null_lprobs[IMM_AMINO_SIZE];
    imm_float null_lodds[IMM_AMINO_SIZE];
    imm_float match_lprobs1[IMM_AMINO_SIZE];
    imm_float match_lprobs2[IMM_AMINO_SIZE];
    imm_float match_lprobs3[IMM_AMINO_SIZE];
    struct dcp_pro_model_trans trans0;
    struct dcp_pro_model_trans trans1;
    struct dcp_pro_model_trans trans2;
    struct dcp_pro_model_trans trans3;
};

struct pro_profile_3core_nodes pro_profile_with_3cores_data(void);
void pro_profile_with_3cores(struct dcp_pro_profile *p);

#endif
