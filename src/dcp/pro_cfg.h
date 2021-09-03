#ifndef DCP_PRO_CFG_H
#define DCP_PRO_CFG_H

#include "dcp/cfg.h"
#include "dcp/entry_dist.h"
#include "dcp/profile_types.h"
#include "imm/imm.h"

struct dcp_pro_cfg
{
    struct dcp_cfg cfg;
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
    return (struct dcp_pro_cfg){
        {DCP_PROTEIN_PROFILE, IMM_FLOAT_BYTES}, amino, nuclt, edist, epsilon};
}

#endif
