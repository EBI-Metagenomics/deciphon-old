#include "std_state.h"
#include "u16toa.h"
#include <assert.h>

unsigned std_state_name(unsigned id, char name[IMM_STATE_NAME_SIZE])
{
  name[0] = 'S';
  assert(id <= UINT16_MAX);
  return u16toa(name + 1, (uint16_t)id) + 1;
}
