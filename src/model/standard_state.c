#include "model/standard_state.h"
#include "to.h"
#include <assert.h>

unsigned standard_state_name(unsigned id, char name[IMM_STATE_NAME_SIZE])
{
    name[0] = 'S';
    assert(id <= UINT16_MAX);
    return to_str(name + 1, (uint16_t)id) + 1;
}
