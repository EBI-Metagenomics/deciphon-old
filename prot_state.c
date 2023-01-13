#include "prot_state.h"
#include "prot_model.h"
#include "u16toa.h"

unsigned prot_state_name(unsigned id, char name[IMM_STATE_NAME_SIZE])
{
  unsigned msb = prot_state_id_msb(id);
  if (msb == PROT_EXT_STATE)
  {
    if (id == PROT_R_STATE)
      name[0] = 'R';
    else if (id == PROT_S_STATE)
      name[0] = 'S';
    else if (id == PROT_N_STATE)
      name[0] = 'N';
    else if (id == PROT_B_STATE)
      name[0] = 'B';
    else if (id == PROT_E_STATE)
      name[0] = 'E';
    else if (id == PROT_J_STATE)
      name[0] = 'J';
    else if (id == PROT_C_STATE)
      name[0] = 'C';
    else if (id == PROT_T_STATE)
      name[0] = 'T';
    name[1] = '\0';
    return 1;
  }
  else
  {
    if (msb == PROT_MATCH_STATE)
      name[0] = 'M';
    else if (msb == PROT_INSERT_STATE)
      name[0] = 'I';
    else if (msb == PROT_DELETE_STATE)
      name[0] = 'D';
    return u16toa(name + 1, (uint16_t)(prot_state_idx(id) + 1)) + 1;
  }
}
