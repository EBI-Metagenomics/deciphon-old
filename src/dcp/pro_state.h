#ifndef DCP_PRO_STATE_H
#define DCP_PRO_STATE_H

#include "dcp/export.h"
#include "dcp/pro_model.h"
#include "imm/imm.h"

static inline unsigned __dcp_pro_state_id_msb(unsigned id)
{
    return id & (3U << (DCP_PROFILE_BITS_ID - 2));
}

static inline bool dcp_pro_state_is_match(unsigned id)
{
    return __dcp_pro_state_id_msb(id) == DCP_PRO_ID_MATCH;
}

static inline bool dcp_pro_state_is_insert(unsigned id)
{
    return __dcp_pro_state_id_msb(id) == DCP_PRO_ID_INSERT;
}

static inline bool dcp_pro_state_is_delete(unsigned id)
{
    return __dcp_pro_state_id_msb(id) == DCP_PRO_ID_DELETE;
}

static inline bool dcp_pro_state_is_mute(unsigned id)
{
    unsigned msb = __dcp_pro_state_id_msb(id);
    return (msb == DCP_PRO_ID_EXT)
               ? ((id == DCP_PRO_ID_S || id == DCP_PRO_ID_B ||
                   id == DCP_PRO_ID_E || id == DCP_PRO_ID_T))
               : msb == DCP_PRO_ID_DELETE;
}

static inline unsigned dcp_pro_state_idx(unsigned id)
{
    return (id & (0xFFFF >> 2)) - 1;
}

DCP_API void dcp_pro_state_name(unsigned id, char name[IMM_STATE_NAME_SIZE]);

#endif
