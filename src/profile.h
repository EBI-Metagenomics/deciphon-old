#ifndef PROFILE_H
#define PROFILE_H

#include "dcp/profile.h"

void profile_init(struct dcp_profile *p, struct imm_abc const *abc,
                  struct dcp_meta mt, struct dcp_profile_vtable vtable);

#endif
