#ifndef DCP_PRO_CFG_H
#define DCP_PRO_CFG_H

#include "dcp/entry_dist.h"
#include "imm/imm.h"

struct dcp_pro_cfg
{
    struct imm_amino const *amino;
    struct imm_nuclt const *nuclt;
    enum dcp_entry_dist edist;
    imm_float epsilon;
};

#define DCP_PRO_CFG_NULL()                                                     \
    {                                                                          \
        NULL, NULL, DCP_ENTRY_DIST_NULL, IMM_LPROB_NAN                         \
    }

#endif
