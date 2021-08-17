#ifndef DCP_PRO_SPECIAL_TRANS_H
#define DCP_PRO_SPECIAL_TRANS_H

#include "imm/imm.h"

struct dcp_pro_special_trans
{
    imm_float NN;
    imm_float CC;
    imm_float JJ;
    imm_float NB;
    imm_float CT;
    imm_float JB;
    imm_float RR;
    imm_float EJ;
    imm_float EC;
};

static inline struct dcp_pro_special_trans dcp_pro_special_trans_init(void)
{
    return (struct dcp_pro_special_trans){.NN = IMM_LPROB_ONE,
                                          .NB = IMM_LPROB_ONE,
                                          .EC = IMM_LPROB_ONE,
                                          .CC = IMM_LPROB_ONE,
                                          .CT = IMM_LPROB_ONE,
                                          .EJ = IMM_LPROB_ONE,
                                          .JJ = IMM_LPROB_ONE,
                                          .JB = IMM_LPROB_ONE,
                                          .RR = IMM_LPROB_ONE};
}

#endif
