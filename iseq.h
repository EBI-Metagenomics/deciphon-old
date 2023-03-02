#ifndef ISEQ_H
#define ISEQ_H

#include "imm/imm.h"

struct iseq
{
  long id;
  char const *name;
  struct imm_seq iseq;
};

struct iseq iseq_init(long id, char const *name, char const *data,
                      struct imm_abc const *);
char const *iseq_data(struct iseq const *);
unsigned iseq_size(struct iseq const *);

#endif
