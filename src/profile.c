#include "profile.h"

void profile_init(struct dcp_prof *p, struct imm_abc const *abc,
                  struct dcp_meta mt, struct dcp_prof_vtable vtable)
{
    p->idx = DCP_PROFILE_NULL_IDX;
    p->abc = abc;
    p->mt = mt;
    p->vtable = vtable;
}
