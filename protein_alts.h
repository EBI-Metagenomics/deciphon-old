#ifndef PROTEIN_ALTS_H
#define PROTEIN_ALTS_H

#include "imm/imm.h"
#include "model.h"

struct protein_alt
{
  struct imm_dp dp;
  unsigned S;
  unsigned N;
  unsigned B;
  unsigned E;
  unsigned J;
  unsigned C;
  unsigned T;
};

struct protein_alts
{
  unsigned core_size;
  struct nuclt_dist *match_nuclt_dists;
  struct nuclt_dist insert_nuclt_dist;
  struct protein_alt zero;
  struct protein_alt full;
};

void protein_alts_init(struct protein_alts *, struct imm_nuclt_code const *);
void protein_alts_setup(struct protein_alts *, struct xtrans const *);
int protein_alts_absorb(struct protein_alts *, struct model *,
                        struct model_summary const *);
int protein_alts_pack(struct protein_alts const *, struct lip_file *);
int protein_alts_unpack(struct protein_alts *, struct lip_file *);
void protein_alts_cleanup(struct protein_alts *);

#endif
