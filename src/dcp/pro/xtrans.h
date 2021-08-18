#ifndef DCP_PRO_XTRANS_H
#define DCP_PRO_XTRANS_H

#include "imm/imm.h"

struct dcp_pro_xtrans
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

static inline struct dcp_pro_xtrans dcp_pro_xtrans_init(void)
{
    return (struct dcp_pro_xtrans){.NN = IMM_LPROB_ONE,
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
