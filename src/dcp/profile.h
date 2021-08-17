#ifndef DCP_PROFILE_H
#define DCP_PROFILE_H

#include "dcp/export.h"
#include "dcp/meta.h"
#include "dcp/profile_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct imm_abc;

struct dcp_profile
{
    unsigned idx;
    struct imm_abc const *abc;
    struct dcp_meta mt;
    struct dcp_profile_vtable vtable;
};

static inline void dcp_profile_del(struct dcp_profile *prof)
{
    if (prof)
        prof->vtable.del(prof);
}

static inline void dcp_profile_nameit(struct dcp_profile *prof,
                                      struct dcp_meta mt)
{
    prof->mt = mt;
}

static inline enum dcp_profile_typeid
dcp_profile_typeid(struct dcp_profile const *prof)
{
    return prof->vtable.typeid;
}

static inline void *dcp_profile_derived(struct dcp_profile *prof)
{
    return prof->vtable.derived;
}

static inline void const *dcp_profile_derived_c(struct dcp_profile const *prof)
{
    return prof->vtable.derived;
}

#endif
