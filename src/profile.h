#ifndef DCP_PROFILE_H
#define DCP_PROFILE_H

#include "dcp/compiler.h"
#include "imm/imm.h"
#include "dcp/rc.h"

struct lip_file;
struct profile;

struct profile_vtable
{
    int typeid;
    void (*del)(struct profile *prof);
    enum rc (*read)(struct profile *prof, struct lip_file *);
    struct imm_dp const *(*null_dp)(struct profile const *prof);
    struct imm_dp const *(*alt_dp)(struct profile const *prof);
};

struct profile
{
    struct profile_vtable vtable;
    imm_state_name *state_name;
    struct imm_code const *code;
    int idx_within_db;
};

void profile_del(struct profile *prof);

enum rc profile_read(struct profile *prof, struct lip_file *cmp);

int profile_typeid(struct profile const *prof);
struct imm_dp const *profile_null_dp(struct profile const *prof);
struct imm_dp const *profile_alt_dp(struct profile const *prof);

void profile_init(struct profile *prof, struct imm_code const *code,
                  struct profile_vtable vtable, imm_state_name *state_name);

void profile_set_state_name(struct profile *prof, imm_state_name *state_name);

#endif
