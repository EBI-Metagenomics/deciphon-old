#ifndef DCP_STD_CFG_H
#define DCP_STD_CFG_H

#include "dcp/profile_types.h"

struct imm_abc;

struct dcp_std_cfg
{
    struct imm_abc const *abc;
};

static inline struct dcp_std_cfg dcp_std_cfg(struct imm_abc const *abc)
{
    return (struct dcp_std_cfg){abc};
}

#endif
