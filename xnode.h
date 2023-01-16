#ifndef XNODE_H
#define XNODE_H

#include "imm/imm.h"

struct xnode_null
{
  struct imm_frame_state R;
};

struct xnode_alt
{
  struct imm_mute_state S;
  struct imm_frame_state N;
  struct imm_mute_state B;
  struct imm_mute_state E;
  struct imm_frame_state J;
  struct imm_frame_state C;
  struct imm_mute_state T;
};

struct xnode
{
  struct xnode_null null;
  struct xnode_alt alt;
};

#endif
