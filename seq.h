#ifndef SEQ_H
#define SEQ_H

#include "imm/imm.h"

struct seq
{
  long id;
  char const *name;
  struct imm_seq iseq;
};

struct seq seq_init(long id, char const *name, char const *data,
                    struct imm_abc const *);
char const *seq_data(struct seq const *);
unsigned seq_size(struct seq const *);

#endif
