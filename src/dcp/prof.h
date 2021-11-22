#ifndef DCP_PROF_H
#define DCP_PROF_H

#include "dcp/export.h"
#include "dcp/meta.h"
#include "dcp/prof_types.h"

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

#endif
