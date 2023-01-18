#ifndef NUCLT_DIST_H
#define NUCLT_DIST_H

#include "deciphon/errno.h"
#include "imm/imm.h"

struct nuclt_dist
{
  struct imm_nuclt_lprob nucltp;
  struct imm_codon_marg codonm;
};

struct lip_file;

void nuclt_dist_init(struct nuclt_dist *, struct imm_nuclt const *);
int nuclt_dist_pack(struct nuclt_dist const *ndist, struct lip_file *);
int nuclt_dist_unpack(struct nuclt_dist *ndist, struct lip_file *);

#endif
