#ifndef PROTEIN_CFG_H
#define PROTEIN_CFG_H

#include "entry_dist.h"
#include "imm/imm.h"

struct dcp_protein_cfg
{
    enum dcp_entry_dist entry_dist;
    imm_float epsilon;
};

#define DCP_DEFAULT_EPSILON ((imm_float)0.01)

static inline struct dcp_protein_cfg dcp_protein_cfg(enum dcp_entry_dist entry_dist,
                                             imm_float epsilon)
{
    assert(epsilon >= 0.0f && epsilon <= 1.0f);
    return (struct dcp_protein_cfg){entry_dist, epsilon};
}

#define DCP_PROTEIN_CFG_DEFAULT                                                    \
    (struct dcp_protein_cfg) { DCP_ENTRY_DIST_OCCUPANCY, DCP_DEFAULT_EPSILON }

#endif
