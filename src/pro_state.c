#include "dcp/pro_state.h"
#include "dcp/pro_model.h"

void dcp_pro_state_name(unsigned id, char name[IMM_STATE_NAME_SIZE])
{
    unsigned msb = __dcp_pro_state_id_msb(id);
    if (msb == DCP_PRO_ID_EXT)
    {
        if (id == DCP_PRO_ID_R)
            name[0] = 'R';
        else if (id == DCP_PRO_ID_S)
            name[0] = 'S';
        else if (id == DCP_PRO_ID_N)
            name[0] = 'N';
        else if (id == DCP_PRO_ID_B)
            name[0] = 'B';
        else if (id == DCP_PRO_ID_E)
            name[0] = 'E';
        else if (id == DCP_PRO_ID_J)
            name[0] = 'J';
        else if (id == DCP_PRO_ID_C)
            name[0] = 'C';
        else if (id == DCP_PRO_ID_T)
            name[0] = 'T';
        name[1] = '\0';
    }
    else
    {
        if (msb == DCP_PRO_ID_MATCH)
            name[0] = 'M';
        else if (msb == DCP_PRO_ID_INSERT)
            name[0] = 'I';
        else if (msb == DCP_PRO_ID_DELETE)
            name[0] = 'D';
        snprintf(name + 1, 7, "%d", dcp_pro_state_idx(id) + 1);
    }
}
