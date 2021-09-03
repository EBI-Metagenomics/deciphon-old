#ifndef DCP_STD_CFG_H
#define DCP_STD_CFG_H

#include "dcp/cfg.h"

struct dcp_std_cfg
{
    struct dcp_cfg cfg;
};

static inline struct dcp_std_cfg dcp_std_cfg(void)
{
    return (struct dcp_std_cfg){{DCP_STD_PROFILE, IMM_FLOAT_BYTES}};
}

#endif
