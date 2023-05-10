#ifndef PROTEIN_NULL_H
#define PROTEIN_NULL_H

#include "imm/imm.h"
#include "model.h"

struct protein_null
{
  struct nuclt_dist nuclt_dist;
  struct imm_dp dp;
  unsigned R;
};

void protein_null_init(struct protein_null *, struct imm_nuclt_code const *);
void protein_null_setup(struct protein_null *, struct xtrans const *);
int protein_null_absorb(struct protein_null *, struct model const *,
                        struct model_summary const *);
int protein_null_pack(struct protein_null const *, struct lip_file *);
int protein_null_unpack(struct protein_null *, struct lip_file *);
void protein_null_cleanup(struct protein_null *);

#endif
