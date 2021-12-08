#include "profile.h"
#include <limits.h>

void profile_del(struct profile *prof)
{
    if (prof) prof->vtable.del(prof);
}

void profile_set_name(struct profile *prof, struct metadata mt)
{
    prof->metadata = mt;
}

int profile_typeid(struct profile const *prof) { return prof->vtable.typeid; }

void profile_init(struct profile *prof, struct imm_code const *code,
                  struct metadata mt, struct profile_vtable vtable)
{
    prof->vtable = vtable;
    /* TODO: it should not be here */
    prof->idx = UINT_MAX;
    prof->code = code;
    prof->metadata = mt;
}
