#ifndef PROFILE_H
#define PROFILE_H

#include "compiler.h"
#include "imm/imm.h"
#include "metadata.h"

enum profile_typeid
{
    PROFILE_NULL,
    PROFILE_STANDARD,
    PROFILE_PROTEIN,
};

struct profile;

struct profile_vtable
{
    void (*del)(struct profile *prof);
    enum profile_typeid typeid;
};

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
