#ifndef MODEL_PROT_STATE_H
#define MODEL_PROT_STATE_H

#include "deciphon_limits.h"
#include "imm/imm.h"

enum prot_state_id
{
    PROTEIN_MATCH_STATE = (0 << (BITS_PER_PROFILE_TYPEID - 2)),
    PROTEIN_INSERT_STATE = (1 << (BITS_PER_PROFILE_TYPEID - 2)),
    PROTEIN_DELETE_STATE = (2 << (BITS_PER_PROFILE_TYPEID - 2)),
    PROTEIN_EXT_STATE = (3 << (BITS_PER_PROFILE_TYPEID - 2)),
    PROTEIN_R_STATE = (PROTEIN_EXT_STATE | 0),
    PROTEIN_S_STATE = (PROTEIN_EXT_STATE | 1),
    PROTEIN_N_STATE = (PROTEIN_EXT_STATE | 2),
    PROTEIN_B_STATE = (PROTEIN_EXT_STATE | 3),
    PROTEIN_E_STATE = (PROTEIN_EXT_STATE | 4),
    PROTEIN_J_STATE = (PROTEIN_EXT_STATE | 5),
    PROTEIN_C_STATE = (PROTEIN_EXT_STATE | 6),
    PROTEIN_T_STATE = (PROTEIN_EXT_STATE | 7),
};

static inline unsigned prot_state_id_msb(unsigned id)
{
    return id & (3U << (BITS_PER_PROFILE_TYPEID - 2));
}

static inline bool prot_state_is_match(unsigned id)
{
    return prot_state_id_msb(id) == PROTEIN_MATCH_STATE;
}

static inline bool prot_state_is_insert(unsigned id)
{
    return prot_state_id_msb(id) == PROTEIN_INSERT_STATE;
}

static inline bool prot_state_is_delete(unsigned id)
{
    return prot_state_id_msb(id) == PROTEIN_DELETE_STATE;
}

static inline bool prot_state_is_mute(unsigned id)
{
    unsigned msb = prot_state_id_msb(id);
    return (msb == PROTEIN_EXT_STATE)
               ? ((id == PROTEIN_S_STATE || id == PROTEIN_B_STATE ||
                   id == PROTEIN_E_STATE || id == PROTEIN_T_STATE))
               : msb == PROTEIN_DELETE_STATE;
}

static inline unsigned prot_state_idx(unsigned id)
{
    return (id & (0xFFFF >> 2)) - 1;
}

unsigned prot_state_name(unsigned id, char name[IMM_STATE_NAME_SIZE]);

#endif
