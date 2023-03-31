#ifndef TRANS_H
#define TRANS_H

#include "imm/imm.h"

enum
{
  TRANS_SIZE = 7
};

struct trans
{
  union
  {
    struct
    {
      float MM;
      float MI;
      float MD;
      float IM;
      float II;
      float DM;
      float DD;
    } __attribute__((packed));
    struct
    {
      float data[TRANS_SIZE];
    };
  };
};

#endif
