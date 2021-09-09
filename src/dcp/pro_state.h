#ifndef DCP_PRO_STATE_H
#define DCP_PRO_STATE_H

#include "dcp/export.h"
#include "imm/imm.h"

static inline unsigned dcp_pro_state_idx(unsigned id)
{
    return id & (0xFFFF >> 2);
}

DCP_API void dcp_pro_state_name(unsigned id, char name[IMM_STATE_NAME_SIZE]);

#endif
