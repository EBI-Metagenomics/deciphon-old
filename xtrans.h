#ifndef XTRANS_H
#define XTRANS_H

#include "imm/imm.h"

struct xtrans
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

void xtrans_init(struct xtrans *);

#endif
