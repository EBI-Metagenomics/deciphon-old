#ifndef PROFILE_H
#define PROFILE_H

#include "meta.h"
#include "profile_types.h"

struct profile
{
    struct profile_vtable vtable;
    unsigned idx;
    struct imm_code const *code;
    struct meta mt;
};

void profile_del(struct profile *prof);

void profile_nameit(struct profile *prof, struct meta mt);

enum profile_typeid profile_typeid(struct profile const *prof);

void profile_init(struct profile *prof, struct imm_code const *code,
                  struct meta mt, struct profile_vtable vtable);

#endif
