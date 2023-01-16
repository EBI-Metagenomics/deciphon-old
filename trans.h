#ifndef TRANS_H
#define TRANS_H

#include "imm/imm.h"

enum
{
  PROT_TRANS_SIZE = 7
};

struct trans
{
  union
  {
    struct
    {
      imm_float MM;
      imm_float MI;
      imm_float MD;
      imm_float IM;
      imm_float II;
      imm_float DM;
      imm_float DD;
    } __attribute__((packed));
    struct
    {
      imm_float data[PROT_TRANS_SIZE];
    };
  };
};

#endif
