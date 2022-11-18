#ifndef DECIPHON_MODEL_PROFILE_H
#define DECIPHON_MODEL_PROFILE_H

#include "deciphon_limits.h"
#include "imm/imm.h"
#include "rc.h"

struct lip_file;
struct profile;

struct profile_vtable
{
    int typeid;
    void (*del)(struct profile *prof);
    enum rc (*unpack)(struct profile *prof, struct lip_file *);
    struct imm_dp const *(*null_dp)(struct profile const *prof);
    struct imm_dp const *(*alt_dp)(struct profile const *prof);
};

struct profile
{
    struct profile_vtable vtable;
    char accession[PROFILE_ACC_SIZE];
    imm_state_name *state_name;
    struct imm_code const *code;
};

void profile_init(struct profile *prof, char const *accession,
                  struct imm_code const *code, struct profile_vtable vtable,
                  imm_state_name *state_name);

void profile_del(struct profile *prof);

enum rc profile_unpack(struct profile *prof, struct lip_file *);

int profile_typeid(struct profile const *prof);

struct imm_dp const *profile_null_dp(struct profile const *prof);
struct imm_dp const *profile_alt_dp(struct profile const *prof);

#endif
