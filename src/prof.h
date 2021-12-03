#ifndef PROF_H
#define PROF_H

#include "meta.h"
#include "prof_types.h"

struct imm_abc;

struct profile
{
    struct dcp_prof_vtable vtable;
    unsigned idx;
    struct imm_code const *code;
    struct dcp_meta mt;
};

static inline void dcp_prof_del(struct profile *prof)
{
    if (prof) prof->vtable.del(prof);
}

static inline void prof_nameit(struct profile *prof, struct dcp_meta mt)
{
    prof->mt = mt;
}

static inline enum dcp_prof_typeid dcp_prof_typeid(struct profile const *prof)
{
    return prof->vtable.typeid;
}

static void profile_init(struct profile *prof, struct imm_code const *code,
                         struct dcp_meta mt, struct dcp_prof_vtable vtable)
{
    prof->vtable = vtable;
    prof->idx = DCP_PROFILE_NULL_IDX;
    prof->code = code;
    prof->mt = mt;
}

#endif
