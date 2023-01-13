#ifndef PROF_H
#define PROF_H

#include "deciphon_limits.h"
#include "imm/imm.h"
#include "rc.h"

struct lip_file;
struct prof;

struct prof_vtable
{
  int typeid;
  void (*del)(struct prof *prof);
  int (*unpack)(struct prof *prof, struct lip_file *);
  struct imm_dp const *(*null_dp)(struct prof const *prof);
  struct imm_dp const *(*alt_dp)(struct prof const *prof);
};

struct prof
{
  struct prof_vtable vtable;
  char acc[PROF_ACC_SIZE];
  imm_state_name *state_name;
  struct imm_code const *code;
};

void prof_init(struct prof *prof, char const *accession,
               struct imm_code const *code, struct prof_vtable vtable,
               imm_state_name *state_name);

void prof_del(struct prof *prof);

int prof_unpack(struct prof *prof, struct lip_file *);

int prof_typeid(struct prof const *prof);

struct imm_dp const *prof_null_dp(struct prof const *prof);
struct imm_dp const *prof_alt_dp(struct prof const *prof);

#endif
