#ifndef PROFILE_H
#define PROFILE_H

#include "compiler.h"
#include "imm/imm.h"
#include "metadata.h"
#include "rc.h"

struct cmp_ctx_s;
struct profile;

struct profile_vtable
{
    int typeid;
    void (*del)(struct profile *prof);
    enum rc (*read)(struct profile *prof, struct cmp_ctx_s *);
    struct imm_dp const *(*null_dp)(struct profile const *prof);
    struct imm_dp const *(*alt_dp)(struct profile const *prof);
};

struct profile
{
    struct profile_vtable vtable;
    unsigned idx;
    struct imm_code const *code;
    struct metadata metadata;
};

void profile_del(struct profile *prof);

enum rc profile_read(struct profile *prof, struct cmp_ctx_s *cmp);

void profile_set_name(struct profile *prof, struct metadata mt);

int profile_typeid(struct profile const *prof);
struct imm_dp const *profile_null_dp(struct profile const *prof);
struct imm_dp const *profile_alt_dp(struct profile const *prof);

void profile_init(struct profile *prof, struct imm_code const *code,
                  struct metadata mt, struct profile_vtable vtable);

#endif
