#ifndef SPECIAL_TRANS_H
#define SPECIAL_TRANS_H

#include "imm/imm.h"

struct special_trans
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

struct special_trans special_trans_get(bool multiple_hits, uint32_t target_length, bool hmmer3_compat);
void                 special_trans_set(struct special_trans trans, struct imm_hmm* hmm, struct imm_dp* dp);

#endif
