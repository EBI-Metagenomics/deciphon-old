#ifndef PROFILE_H
#define PROFILE_H

#include "compiler.h"
#include "imm/imm.h"
#include "metadata.h"

struct profile;

struct profile_vtable
{
    void (*del)(struct profile *prof);
    int typeid;
};

struct profile
{
    struct profile_vtable vtable;
    unsigned idx;
    struct imm_code const *code;
    struct metadata metadata;
};

void profile_del(struct profile *prof);

void profile_set_name(struct profile *prof, struct metadata mt);

int profile_typeid(struct profile const *prof);

void profile_init(struct profile *prof, struct imm_code const *code,
                  struct metadata mt, struct profile_vtable vtable);

#endif
