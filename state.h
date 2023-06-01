#ifndef STATE_H
#define STATE_H

#include "imm/imm.h"

enum
{
  STATE_ID_BITS = 16
};

enum state_id
{
  STATE_MATCH = (0 << (STATE_ID_BITS - 2)),
  STATE_INSERT = (1 << (STATE_ID_BITS - 2)),
  STATE_DELETE = (2 << (STATE_ID_BITS - 2)),
  STATE_EXT = (3 << (STATE_ID_BITS - 2)),
  STATE_R = (STATE_EXT | 0),
  STATE_S = (STATE_EXT | 1),
  STATE_N = (STATE_EXT | 2),
  STATE_B = (STATE_EXT | 3),
  STATE_E = (STATE_EXT | 4),
  STATE_J = (STATE_EXT | 5),
  STATE_C = (STATE_EXT | 6),
  STATE_T = (STATE_EXT | 7),
};

unsigned state_id_msb(unsigned id);
bool state_is_match(unsigned id);
bool state_is_insert(unsigned id);
bool state_is_delete(unsigned id);
bool state_is_mute(unsigned id);
unsigned state_idx(unsigned id);
char *state_name(unsigned id, char name[IMM_STATE_NAME_SIZE]);

#endif
