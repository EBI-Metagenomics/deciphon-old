#ifndef DCP_NORMAL_PROFILE_H
#define DCP_NORMAL_PROFILE_H

#include "dcp/export.h"
#include "dcp/profile.h"

struct imm_dcp;

struct dcp_normal_profile
{
    struct dcp_profile *super;
    struct
    {
        struct imm_dp *null;
        struct imm_dp *alt;
    } dp;
};

DCP_API struct dcp_normal_profile *
dcp_normal_profile_new(struct imm_abc const *abc, struct dcp_metadata mt,
                       struct imm_dp *null, struct imm_dp *alt);

static inline void dcp_normal_profile_del(struct dcp_normal_profile const *prof)
{
    if (prof)
    {
        prof->super->vtable.del(prof->super);
    }
}

#endif
