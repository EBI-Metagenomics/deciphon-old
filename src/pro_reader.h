#ifndef PRO_READER_H
#define PRO_READER_H

#include "hmr/hmr.h"
#include "imm/imm.h"
#include "meta.h"
#include "pro_cfg.h"
#include "pro_model.h"
#include "rc.h"
#include <stdio.h>

struct dcp_pro_reader
{
    struct hmr hmr;
    struct hmr_prof prof;
    imm_float null_lprobs[IMM_AMINO_SIZE];
    struct dcp_pro_model model;
};

void dcp_pro_reader_init(struct dcp_pro_reader *reader,
                         struct imm_amino const *amino,
                         struct imm_nuclt_code const *code,
                         struct dcp_pro_cfg cfg, FILE *restrict fd);

enum dcp_rc dcp_pro_reader_next(struct dcp_pro_reader *reader);

struct dcp_meta dcp_pro_reader_meta(struct dcp_pro_reader const *r);

#endif
