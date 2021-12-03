#ifndef PRO_STATE_H
#define PRO_STATE_H

#include "imm/imm.h"
#include "pro_id.h"

static inline unsigned pro_state_id_msb(unsigned id)
{
    return id & (3U << (DCP_PROFILE_BITS_ID - 2));
}

static inline bool dcp_pro_state_is_match(unsigned id)
{
    return pro_state_id_msb(id) == DCP_PRO_ID_MATCH;
}

static inline bool dcp_pro_state_is_insert(unsigned id)
{
    return pro_state_id_msb(id) == DCP_PRO_ID_INSERT;
}

static inline bool dcp_pro_state_is_delete(unsigned id)
{
    return pro_state_id_msb(id) == DCP_PRO_ID_DELETE;
}

static inline bool dcp_pro_state_is_mute(unsigned id)
{
    unsigned msb = pro_state_id_msb(id);
    return (msb == DCP_PRO_ID_EXT)
               ? ((id == DCP_PRO_ID_S || id == DCP_PRO_ID_B ||
                   id == DCP_PRO_ID_E || id == DCP_PRO_ID_T))
               : msb == DCP_PRO_ID_DELETE;
}

static inline unsigned dcp_pro_state_idx(unsigned id)
{
    return (id & (0xFFFF >> 2)) - 1;
}

unsigned dcp_pro_state_name(unsigned id, char name[IMM_STATE_NAME_SIZE]);

#endif
