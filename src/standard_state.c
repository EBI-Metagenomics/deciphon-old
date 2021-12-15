#include "standard_state.h"

unsigned standard_state_name(unsigned id, char name[IMM_STATE_NAME_SIZE])
{
    name[0] = 'S';
    name[1] = 0;
    return 1;
}
