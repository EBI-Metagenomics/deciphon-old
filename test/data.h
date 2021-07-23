#ifndef TEST_DATA_H
#define TEST_DATA_H

#include "dcp/dcp.h"
#include "imm/imm.h"

struct pp_3core_nodes
{
    struct imm_amino amino;
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

struct pp_3core_nodes pp_3core_nodes_data(void);

#endif
