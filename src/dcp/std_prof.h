#ifndef DCP_STD_PROF_H
#define DCP_STD_PROF_H

#include "dcp/export.h"
#include "dcp/prof.h"

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

DCP_API void dcp_std_prof_init(struct dcp_std_prof *, struct imm_abc const *);

static inline void dcp_std_prof_del(struct dcp_std_prof *prof)
{
    if (prof) prof->super.vtable.del(&prof->super);
}

static inline struct dcp_prof *dcp_std_prof_super(struct dcp_std_prof *std)
{
    return &std->super;
}

#endif
