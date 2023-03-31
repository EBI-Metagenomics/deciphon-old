#ifndef H3READER_H
#define H3READER_H

#include "cfg.h"
#include "deciphon/errno.h"
#include "hmr/hmr.h"
#include "imm/imm.h"
#include "model.h"
#include <stdio.h>

struct h3reader
{
  struct hmr hmr;
  struct hmr_prof protein;
  float null_lprobs[IMM_AMINO_SIZE];
  struct model model;
  bool end;
};

void h3reader_init(struct h3reader *, struct imm_amino const *,
                   struct imm_nuclt_code const *, struct cfg, FILE *);
int h3reader_next(struct h3reader *);
bool h3reader_end(struct h3reader const *);
void h3reader_del(struct h3reader const *);

#endif
