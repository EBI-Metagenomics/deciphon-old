#ifndef DCP_STD_PROFILE_H
#define DCP_STD_PROFILE_H

#include "dcp/export.h"
#include "dcp/profile.h"

struct imm_dcp;

struct dcp_std_prof
{
    struct dcp_prof super;
    struct
    {
        struct imm_dp null;
        struct imm_dp alt;
    } dp;
};

DCP_API void dcp_std_prof_init(struct dcp_std_prof *p,
                               struct imm_abc const *abc);

static inline void dcp_std_prof_del(struct dcp_std_prof *prof)
{
    if (prof) prof->super.vtable.del(&prof->super);
}

static inline struct dcp_prof *dcp_std_prof_super(struct dcp_std_prof *std)
{
    return &std->super;
}

#endif
