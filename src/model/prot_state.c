#include "model/prot_state.h"
#include "model/prot_model.h"
#include "to.h"

unsigned prot_state_name(unsigned id, char name[IMM_STATE_NAME_SIZE])
{
    unsigned msb = prot_state_id_msb(id);
    if (msb == PROTEIN_EXT_STATE)
    {
        if (id == PROTEIN_R_STATE)
            name[0] = 'R';
        else if (id == PROTEIN_S_STATE)
            name[0] = 'S';
        else if (id == PROTEIN_N_STATE)
            name[0] = 'N';
        else if (id == PROTEIN_B_STATE)
            name[0] = 'B';
        else if (id == PROTEIN_E_STATE)
            name[0] = 'E';
        else if (id == PROTEIN_J_STATE)
            name[0] = 'J';
        else if (id == PROTEIN_C_STATE)
            name[0] = 'C';
        else if (id == PROTEIN_T_STATE)
            name[0] = 'T';
        name[1] = '\0';
        return 1;
    }
    else
    {
        if (msb == PROTEIN_MATCH_STATE)
            name[0] = 'M';
        else if (msb == PROTEIN_INSERT_STATE)
            name[0] = 'I';
        else if (msb == PROTEIN_DELETE_STATE)
            name[0] = 'D';
        return to_str(name + 1, (uint16_t)(prot_state_idx(id) + 1)) + 1;
    }
}
