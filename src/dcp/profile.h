#ifndef DCP_PROFILE_H
#define DCP_PROFILE_H

#include "dcp/export.h"
#include "dcp/meta.h"
#include "dcp/profile_types.h"

struct imm_abc;

struct dcp_prof
{
    unsigned idx;
    struct imm_abc const *abc;
    struct dcp_meta mt;
    struct dcp_prof_vtable vtable;
};

static inline void dcp_prof_del(struct dcp_prof *prof)
{
    if (prof) prof->vtable.del(prof);
}

static inline void dcp_prof_nameit(struct dcp_prof *prof, struct dcp_meta mt)
{
    prof->mt = mt;
}

static inline enum dcp_prof_typeid dcp_prof_typeid(struct dcp_prof const *prof)
{
    return prof->vtable.typeid;
}

static inline void *dcp_prof_derived(struct dcp_prof *prof)
{
    return prof->vtable.derived;
}

static inline void const *dcp_prof_derived_c(struct dcp_prof const *prof)
{
    return prof->vtable.derived;
}

#endif
