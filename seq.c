#include "seq.h"

struct seq seq_init(long id, char const *name, char const *data,
                    struct imm_abc const *abc)
{
  return (struct seq){id, name, imm_seq(imm_str(data), abc)};
}

char const *seq_data(struct seq const *x) { return imm_seq_str(&x->iseq); }

unsigned seq_size(struct seq const *x) { return imm_seq_size(&x->iseq); }
