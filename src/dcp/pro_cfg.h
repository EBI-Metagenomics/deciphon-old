#ifndef DCP_PRO_CFG_H
#define DCP_PRO_CFG_H

#include "dcp/entry_dist.h"
#include "dcp/profile_types.h"

struct imm_amino;
struct imm_nuclt;

struct dcp_pro_cfg
{
    struct imm_amino const *amino;
    struct imm_nuclt const *nuclt;
    enum dcp_entry_dist edist;
    imm_float epsilon;
};

static inline struct dcp_pro_cfg dcp_pro_cfg(struct imm_amino const *amino,
                                             struct imm_nuclt const *nuclt,
                                             enum dcp_entry_dist edist,
                                             imm_float epsilon)
{
    return (struct dcp_pro_cfg){amino, nuclt, edist, epsilon};
}

#endif
