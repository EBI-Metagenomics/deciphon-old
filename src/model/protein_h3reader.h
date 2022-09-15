#ifndef DECIPHON_MODEL_PROTEIN_H3READER_H
#define DECIPHON_MODEL_PROTEIN_H3READER_H

#include "core/rc.h"
#include "hmr/hmr.h"
#include "imm/imm.h"
#include "model/protein_cfg.h"
#include "model/protein_model.h"
#include <stdio.h>

struct protein_h3reader
{
    struct hmr hmr;
    struct hmr_prof prof;
    imm_float null_lprobs[IMM_AMINO_SIZE];
    struct protein_model model;
};

void protein_h3reader_init(struct protein_h3reader *reader,
                           struct imm_amino const *amino,
                           struct imm_nuclt_code const *code,
                           struct protein_cfg cfg, FILE *fp);

enum rc protein_h3reader_next(struct protein_h3reader *reader);
void protein_h3reader_del(struct protein_h3reader const *reader);

#endif
