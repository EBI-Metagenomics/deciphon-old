#ifndef H3READER_H
#define H3READER_H

#include "cfg.h"
#include "hmr/hmr.h"
#include "imm/imm.h"
#include "model.h"
#include "rc.h"
#include <stdio.h>

struct h3reader
{
  struct hmr hmr;
  struct hmr_prof prof;
  imm_float null_lprobs[IMM_AMINO_SIZE];
  struct model model;
  bool end;
};

void h3reader_init(struct h3reader *reader, struct imm_amino const *amino,
                   struct imm_nuclt_code const *code, struct cfg cfg, FILE *fp);

int h3reader_next(struct h3reader *reader);
bool h3reader_end(struct h3reader const *reader);
void h3reader_del(struct h3reader const *reader);

#endif
