#ifndef PROFILE_H
#define PROFILE_H

#include "meta.h"
#include "profile_types.h"

struct imm_abc;

struct profile
{
    struct profile_vtable vtable;
    unsigned idx;
    struct imm_code const *code;
    struct meta mt;
};

static inline void profile_del(struct profile *prof)
{
    if (prof) prof->vtable.del(prof);
}

static inline void profile_nameit(struct profile *prof, struct meta mt)
{
    prof->mt = mt;
}

static inline enum profile_typeid profile_typeid(struct profile const *prof)
{
    return prof->vtable.typeid;
}

static void profile_init(struct profile *prof, struct imm_code const *code,
                         struct meta mt, struct profile_vtable vtable)
{
    prof->vtable = vtable;
    prof->idx = PROFILE_NULL_IDX;
    prof->code = code;
    prof->mt = mt;
}

#endif
