#include "deciphon/model/profile.h"
#include "deciphon/compiler.h"
#include "deciphon/util/strlcpy.h"

void profile_del(struct profile *prof)
{
    if (prof) prof->vtable.del(prof);
}

enum rc profile_unpack(struct profile *prof, struct lip_file *file)
{
    return prof->vtable.unpack(prof, file);
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

void profile_init(struct profile *prof, char const *accession,
                  struct imm_code const *code, struct profile_vtable vtable,
                  imm_state_name *state_name)
{
    prof->vtable = vtable;
    strlcpy(prof->accession, accession, ARRAY_SIZE_OF(*prof, accession));
    prof->state_name = state_name;
    prof->code = code;
}
