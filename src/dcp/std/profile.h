#ifndef DCP_STD_PROFILE_H
#define DCP_STD_PROFILE_H

#include "dcp/export.h"
#include "dcp/profile.h"

struct imm_dcp;

struct dcp_std_profile
{
    struct dcp_profile super;
    struct
    {
        struct imm_dp null;
        struct imm_dp alt;
    } dp;
};

DCP_API void dcp_std_profile_init(struct dcp_std_profile *p,
                                  struct imm_abc const *abc);

static inline void dcp_std_profile_del(struct dcp_std_profile *prof)
{
    if (prof)
        prof->super.vtable.del(&prof->super);
}

static inline struct dcp_profile *
dcp_std_profile_super(struct dcp_std_profile *std)
{
    return &std->super;
}

#endif
