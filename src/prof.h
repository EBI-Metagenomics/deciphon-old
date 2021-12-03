#ifndef PROF_H
#define PROF_H

#include "meta.h"
#include "prof_types.h"

struct imm_abc;

struct dcp_prof
{
    struct dcp_prof_vtable vtable;
    unsigned idx;
    struct imm_code const *code;
    struct dcp_meta mt;
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

static void profile_init(struct dcp_prof *prof, struct imm_code const *code,
                         struct dcp_meta mt, struct dcp_prof_vtable vtable)
{
    prof->vtable = vtable;
    prof->idx = DCP_PROFILE_NULL_IDX;
    prof->code = code;
    prof->mt = mt;
}

#endif
