#ifndef PROTEIN_READER_H
#define PROTEIN_READER_H

#include "hmr/hmr.h"
#include "imm/imm.h"
#include "meta.h"
#include "protein_cfg.h"
#include "protein_model.h"
#include "rc.h"
#include <stdio.h>

struct protein_reader
{
    struct hmr hmr;
    struct hmr_prof prof;
    imm_float null_lprobs[IMM_AMINO_SIZE];
    struct protein_model model;
};

void protein_reader_init(struct protein_reader *reader,
                         struct imm_amino const *amino,
                         struct imm_nuclt_code const *code,
                         struct protein_cfg cfg, FILE *restrict fd);

enum rc protein_reader_next(struct protein_reader *reader);

struct meta protein_reader_meta(struct protein_reader const *r);

#endif
