#include "profile.h"
#include <limits.h>

void profile_del(struct profile *prof)
{
    if (prof) prof->vtable.del(prof);
}

enum rc profile_read(struct profile *prof, struct cmp_ctx_s *cmp)
{
    return prof->vtable.read(prof, cmp);
}

void profile_set_name(struct profile *prof, struct metadata mt)
{
    prof->metadata = mt;
}

int profile_typeid(struct profile const *prof) { return prof->vtable.typeid; }

struct imm_dp const *profile_null_dp(struct profile const *prof)
{
    return prof->vtable.null_dp(prof);
}

struct imm_dp const *profile_alt_dp(struct profile const *prof)
{
    return prof->vtable.alt_dp(prof);
}

void profile_init(struct profile *prof, struct imm_code const *code,
                  struct metadata mt, struct profile_vtable vtable,
                  imm_state_name *state_name)
{
    prof->vtable = vtable;
    prof->state_name = state_name;
    prof->code = code;
    prof->metadata = mt;
}

void profile_set_state_name(struct profile *prof, imm_state_name *state_name)
{
    prof->state_name = state_name;
}
