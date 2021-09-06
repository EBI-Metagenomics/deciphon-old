#ifndef DCP_PRO_CFG_H
#define DCP_PRO_CFG_H

#include "dcp/entry_dist.h"
#include "imm/imm.h"

struct dcp_pro_cfg
{
    enum dcp_entry_dist entry_dist;
    imm_float epsilon;
};

static inline struct dcp_pro_cfg dcp_pro_cfg(enum dcp_entry_dist entry_dist,
                                             imm_float epsilon)
{
    assert(epsilon >= 0.0f && epsilon <= 1.0f);
    return (struct dcp_pro_cfg){entry_dist, epsilon};
}

#define DCP_PRO_CFG_DEFAULT                                                    \
    (struct dcp_pro_cfg) { DCP_ENTRY_DIST_OCCUPANCY, 0.1f }

#endif
