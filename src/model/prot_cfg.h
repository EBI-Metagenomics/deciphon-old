#ifndef MODEL_PROT_CFG_H
#define MODEL_PROT_CFG_H

#include "imm/imm.h"
#include "model/entry_dist.h"

struct prot_cfg
{
    enum entry_dist entry_dist;
    imm_float epsilon;
};

#define DEFAULT_EPSILON ((imm_float)0.01)

static inline struct prot_cfg prot_cfg(enum entry_dist entry_dist,
                                       imm_float epsilon)
{
    assert(epsilon >= 0.0f && epsilon <= 1.0f);
    return (struct prot_cfg){entry_dist, epsilon};
}

#define PROTEIN_CFG_DEFAULT                                                    \
    (struct prot_cfg)                                                          \
    {                                                                          \
        ENTRY_DIST_OCCUPANCY, DEFAULT_EPSILON                                  \
    }

#endif
