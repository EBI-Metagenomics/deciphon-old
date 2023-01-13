#include "prof.h"
#include "compiler.h"
#include "strlcpy.h"

void prof_del(struct prof *prof)
{
  if (prof) prof->vtable.del(prof);
}

int prof_unpack(struct prof *prof, struct lip_file *file)
{
  return prof->vtable.unpack(prof, file);
}

int prof_typeid(struct prof const *prof) { return prof->vtable.typeid; }

struct imm_dp const *prof_null_dp(struct prof const *prof)
{
  return prof->vtable.null_dp(prof);
}

struct imm_dp const *prof_alt_dp(struct prof const *prof)
{
  return prof->vtable.alt_dp(prof);
}

void prof_init(struct prof *prof, char const *accession,
               struct imm_code const *code, struct prof_vtable vtable,
               imm_state_name *state_name)
{
  prof->vtable = vtable;
  strlcpy(prof->acc, accession, PROF_ACC_SIZE);
  prof->state_name = state_name;
  prof->code = code;
}
