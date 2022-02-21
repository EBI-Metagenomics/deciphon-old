#ifndef DCP_PROTEIN_CFG_H
#define DCP_PROTEIN_CFG_H

#include "entry_dist.h"
#include "imm/imm.h"

struct protein_cfg
{
    enum entry_dist entry_dist;
    imm_float epsilon;
};

#define DEFAULT_EPSILON ((imm_float)0.01)

static inline struct protein_cfg protein_cfg(enum entry_dist entry_dist,
                                             imm_float epsilon)
{
    assert(epsilon >= 0.0f && epsilon <= 1.0f);
    return (struct protein_cfg){entry_dist, epsilon};
}

#define PROTEIN_CFG_DEFAULT                                                    \
    (struct protein_cfg) { ENTRY_DIST_OCCUPANCY, DEFAULT_EPSILON }

#endif
