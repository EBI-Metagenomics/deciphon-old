#ifndef NODE_H
#define NODE_H

#include "imm/imm.h"
#include "nuclt_dist.h"

struct node
{
  union
  {
    struct imm_frame_state M;
    struct
    {
      struct imm_frame_state state;
      struct nuclt_dist nucltd;
    } match;
  };
  struct imm_frame_state I;
  struct imm_mute_state D;
};

#endif
