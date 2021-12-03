#ifndef PROTEIN_STATE_H
#define PROTEIN_STATE_H

#include "imm/imm.h"
#include "protein_id.h"

static inline unsigned protein_state_id_msb(unsigned id)
{
    return id & (3U << (DCP_PROFILE_BITS_ID - 2));
}

static inline bool dcp_protein_state_is_match(unsigned id)
{
    return protein_state_id_msb(id) == DCP_PROTEIN_ID_MATCH;
}

static inline bool dcp_protein_state_is_insert(unsigned id)
{
    return protein_state_id_msb(id) == DCP_PROTEIN_ID_INSERT;
}

static inline bool dcp_protein_state_is_delete(unsigned id)
{
    return protein_state_id_msb(id) == DCP_PROTEIN_ID_DELETE;
}

static inline bool dcp_protein_state_is_mute(unsigned id)
{
    unsigned msb = protein_state_id_msb(id);
    return (msb == DCP_PROTEIN_ID_EXT)
               ? ((id == DCP_PROTEIN_ID_S || id == DCP_PROTEIN_ID_B ||
                   id == DCP_PROTEIN_ID_E || id == DCP_PROTEIN_ID_T))
               : msb == DCP_PROTEIN_ID_DELETE;
}

static inline unsigned dcp_protein_state_idx(unsigned id)
{
    return (id & (0xFFFF >> 2)) - 1;
}

unsigned dcp_protein_state_name(unsigned id, char name[IMM_STATE_NAME_SIZE]);

#endif
