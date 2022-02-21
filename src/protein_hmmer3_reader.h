#ifndef DCP_PROTEIN_HMMER3_READER_H
#define DCP_PROTEIN_HMMER3_READER_H

#include "common/rc.h"
#include "hmr/hmr.h"
#include "imm/imm.h"
#include "metadata.h"
#include "protein_cfg.h"
#include "protein_model.h"
#include <stdio.h>

struct protein_hmmer3_reader
{
    struct hmr hmr;
    struct hmr_prof prof;
    imm_float null_lprobs[IMM_AMINO_SIZE];
    struct protein_model model;
};

void protein_hmmer3_reader_init(struct protein_hmmer3_reader *reader,
                                struct imm_amino const *amino,
                                struct imm_nuclt_code const *code,
                                struct protein_cfg cfg, FILE *fp);

enum rc protein_hmmer3_reader_next(struct protein_hmmer3_reader *reader);
void protein_hmmer3_reader_del(struct protein_hmmer3_reader const *reader);

struct metadata
protein_hmmer3_reader_metadata(struct protein_hmmer3_reader const *r);

#endif
