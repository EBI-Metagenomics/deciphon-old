#ifndef DCP_PRO_READER_H
#define DCP_PRO_READER_H

#include "dcp/export.h"
#include "dcp/pro_cfg.h"
#include "dcp/pro_model.h"
#include "dcp/rc.h"
#include "hmr/hmr.h"
#include "imm/imm.h"
#include <stdio.h>

struct dcp_pro_reader
{
    struct hmr hmr;
    struct hmr_prof prof;
    imm_float null_lprobs[IMM_AMINO_SIZE];
    struct dcp_pro_model model;
};

DCP_API void dcp_pro_reader_init(struct dcp_pro_reader *reader,
                                 struct imm_amino const *amino,
                                 struct imm_nuclt const *nuclt,
                                 struct dcp_pro_cfg cfg, FILE *restrict fd);

DCP_API enum dcp_rc dcp_pro_reader_next(struct dcp_pro_reader *reader);

#endif
