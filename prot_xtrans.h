#ifndef MODEL_PROT_XTRANS_H
#define MODEL_PROT_XTRANS_H

#include "imm/imm.h"

struct prot_xtrans
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

static inline void prot_xtrans_init(struct prot_xtrans *t)
{
  t->NN = IMM_LPROB_ONE;
  t->NB = IMM_LPROB_ONE;
  t->EC = IMM_LPROB_ONE;
  t->CC = IMM_LPROB_ONE;
  t->CT = IMM_LPROB_ONE;
  t->EJ = IMM_LPROB_ONE;
  t->JJ = IMM_LPROB_ONE;
  t->JB = IMM_LPROB_ONE;
  t->RR = IMM_LPROB_ONE;
}

#endif
