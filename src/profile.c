#include "profile.h"
#include <limits.h>

void profile_del(struct profile *prof)
{
    if (prof) prof->vtable.del(prof);
}

void profile_nameit(struct profile *prof, struct metadata mt) { prof->mt = mt; }

enum profile_typeid profile_typeid(struct profile const *prof)
{
    return prof->vtable.typeid;
}

void profile_init(struct profile *prof, struct imm_code const *code,
                  struct metadata mt, struct profile_vtable vtable)
{
    prof->vtable = vtable;
    prof->idx = UINT_MAX;
    prof->code = code;
    prof->mt = mt;
}
