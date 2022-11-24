#ifndef MODEL_PROT_H3READER_H
#define MODEL_PROT_H3READER_H

#include "hmr/hmr.h"
#include "imm/imm.h"
#include "model/prot_cfg.h"
#include "model/prot_model.h"
#include "rc.h"
#include <stdio.h>

struct prot_h3reader
{
    struct hmr hmr;
    struct hmr_prof prof;
    imm_float null_lprobs[IMM_AMINO_SIZE];
    struct prot_model model;
};

void prot_h3reader_init(struct prot_h3reader *reader,
                        struct imm_amino const *amino,
                        struct imm_nuclt_code const *code, struct prot_cfg cfg,
                        FILE *fp);

enum rc prot_h3reader_next(struct prot_h3reader *reader);
void prot_h3reader_del(struct prot_h3reader const *reader);

#endif
