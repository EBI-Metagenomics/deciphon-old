#ifndef PROTEIN_READER_H
#define PROTEIN_READER_H

#include "hmr/hmr.h"
#include "imm/imm.h"
#include "metadata.h"
#include "protein_cfg.h"
#include "protein_model.h"
#include "rc.h"
#include <stdio.h>

/* TODO: change this name to something like
 * hmmer3_protein_reader. */
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
void protein_reader_del(struct protein_reader const *reader);

struct metadata protein_reader_metadata(struct protein_reader const *r);

#endif
