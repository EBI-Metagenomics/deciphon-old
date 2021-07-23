#ifndef TEST_DATA_H
#define TEST_DATA_H

#include "dcp/dcp.h"
#include "imm/imm.h"

struct pro_profile_3core_nodes
{
    struct imm_amino const *amino;
    struct imm_dna const *dna;
    unsigned core_size;
    enum dcp_entry_distr edistr;
    imm_float epsilon;
    imm_float null_lprobs[IMM_AMINO_SIZE];
    imm_float null_lodds[IMM_AMINO_SIZE];
    imm_float match_lprobs1[20];
    imm_float match_lprobs2[20];
    imm_float match_lprobs3[20];
    struct dcp_pro_model_trans trans0;
    struct dcp_pro_model_trans trans1;
    struct dcp_pro_model_trans trans2;
    struct dcp_pro_model_trans trans3;
};

struct pro_profile_3core_nodes pro_profile_with_3cores_data(void);
struct dcp_pro_profile *pro_profile_with_3cores(void);

#endif
