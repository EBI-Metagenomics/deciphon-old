#ifndef PROFILE_H
#define PROFILE_H

#include "dcp/profile.h"

void profile_init(struct dcp_prof *p, struct imm_abc const *abc,
                  struct dcp_meta mt, struct dcp_prof_vtable vtable);

#endif
