#ifndef STATE_H
#define STATE_H

#include "deciphon_limits.h"
#include "imm/imm.h"

enum prot_state_id
{
  PROT_MATCH_STATE = (0 << (BITS_PER_PROF_TYPEID - 2)),
  PROT_INSERT_STATE = (1 << (BITS_PER_PROF_TYPEID - 2)),
  PROT_DELETE_STATE = (2 << (BITS_PER_PROF_TYPEID - 2)),
  PROT_EXT_STATE = (3 << (BITS_PER_PROF_TYPEID - 2)),
  PROT_R_STATE = (PROT_EXT_STATE | 0),
  PROT_S_STATE = (PROT_EXT_STATE | 1),
  PROT_N_STATE = (PROT_EXT_STATE | 2),
  PROT_B_STATE = (PROT_EXT_STATE | 3),
  PROT_E_STATE = (PROT_EXT_STATE | 4),
  PROT_J_STATE = (PROT_EXT_STATE | 5),
  PROT_C_STATE = (PROT_EXT_STATE | 6),
  PROT_T_STATE = (PROT_EXT_STATE | 7),
};

static inline unsigned state_id_msb(unsigned id)
{
  return id & (3U << (BITS_PER_PROF_TYPEID - 2));
}

static inline bool state_is_match(unsigned id)
{
  return state_id_msb(id) == PROT_MATCH_STATE;
}

static inline bool state_is_insert(unsigned id)
{
  return state_id_msb(id) == PROT_INSERT_STATE;
}

static inline bool state_is_delete(unsigned id)
{
  return state_id_msb(id) == PROT_DELETE_STATE;
}

static inline bool state_is_mute(unsigned id)
{
  unsigned msb = state_id_msb(id);
  return (msb == PROT_EXT_STATE) ? ((id == PROT_S_STATE || id == PROT_B_STATE ||
                                     id == PROT_E_STATE || id == PROT_T_STATE))
                                 : msb == PROT_DELETE_STATE;
}

static inline unsigned state_idx(unsigned id)
{
  return (id & (0xFFFF >> 2)) - 1;
}

unsigned state_name(unsigned id, char name[IMM_STATE_NAME_SIZE]);

#endif
