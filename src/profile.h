#ifndef PROFILE_H
#define PROFILE_H

#include "metadata.h"
#include "profile_types.h"

struct profile
{
    struct profile_vtable vtable;
    unsigned idx;
    struct imm_code const *code;
    struct metadata mt;
};

void profile_del(struct profile *prof);

void profile_nameit(struct profile *prof, struct metadata mt);

enum profile_typeid profile_typeid(struct profile const *prof);

void profile_init(struct profile *prof, struct imm_code const *code,
                  struct metadata mt, struct profile_vtable vtable);

#endif
