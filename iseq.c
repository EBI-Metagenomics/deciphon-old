#include "iseq.h"

struct iseq iseq_init(long id, char const *name, char const *data,
                      struct imm_abc const *abc)
{
  return (struct iseq){id, name, imm_seq(imm_str(data), abc)};
}

char const *iseq_data(struct iseq const *x) { return imm_seq_str(&x->iseq); }

unsigned iseq_size(struct iseq const *x) { return imm_seq_size(&x->iseq); }
