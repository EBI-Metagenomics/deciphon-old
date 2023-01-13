#ifndef MODEL_PROT_H3READER_H
#define MODEL_PROT_H3READER_H

#include "hmr/hmr.h"
#include "imm/imm.h"
#include "prot_cfg.h"
#include "prot_model.h"
#include "rc.h"
#include <stdio.h>

struct prot_h3reader
{
  struct hmr hmr;
  struct hmr_prof prof;
  imm_float null_lprobs[IMM_AMINO_SIZE];
  struct prot_model model;
  bool end;
};

void prot_h3reader_init(struct prot_h3reader *reader,
                        struct imm_amino const *amino,
                        struct imm_nuclt_code const *code, struct prot_cfg cfg,
                        FILE *fp);

int prot_h3reader_next(struct prot_h3reader *reader);
bool prot_h3reader_end(struct prot_h3reader const *reader);
void prot_h3reader_del(struct prot_h3reader const *reader);

#endif
