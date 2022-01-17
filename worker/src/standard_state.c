#include "standard_state.h"
#include "common/to.h"

unsigned standard_state_name(unsigned id, char name[IMM_STATE_NAME_SIZE])
{
    name[0] = 'S';
    return to_str(name + 1, (uint16_t)id) + 1;
}
