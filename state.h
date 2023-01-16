#ifndef STATE_H
#define STATE_H

#include "imm/imm.h"

enum
{
  BITS_PER_STATE_ID = 16
};

enum state_id
{
  PROT_MATCH_STATE = (0 << (BITS_PER_STATE_ID - 2)),
  PROT_INSERT_STATE = (1 << (BITS_PER_STATE_ID - 2)),
  PROT_DELETE_STATE = (2 << (BITS_PER_STATE_ID - 2)),
  PROT_EXT_STATE = (3 << (BITS_PER_STATE_ID - 2)),
  PROT_R_STATE = (PROT_EXT_STATE | 0),
  PROT_S_STATE = (PROT_EXT_STATE | 1),
  PROT_N_STATE = (PROT_EXT_STATE | 2),
  PROT_B_STATE = (PROT_EXT_STATE | 3),
  PROT_E_STATE = (PROT_EXT_STATE | 4),
  PROT_J_STATE = (PROT_EXT_STATE | 5),
  PROT_C_STATE = (PROT_EXT_STATE | 6),
  PROT_T_STATE = (PROT_EXT_STATE | 7),
};

unsigned state_id_msb(unsigned id);
bool state_is_match(unsigned id);
bool state_is_insert(unsigned id);
bool state_is_delete(unsigned id);
bool state_is_mute(unsigned id);
unsigned state_idx(unsigned id);
unsigned state_name(unsigned id, char name[IMM_STATE_NAME_SIZE]);

#endif
