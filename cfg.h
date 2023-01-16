#ifndef CFG_H
#define CFG_H

#include "entry_dist.h"
#include "imm/imm.h"

struct cfg
{
  enum entry_dist edist;
  imm_float eps;
};

#define CFG_DEFAULT                                                            \
  (struct cfg)                                                                 \
  {                                                                            \
    ENTRY_DIST_OCCUPANCY, (imm_float)0.01                                      \
  }

#endif
