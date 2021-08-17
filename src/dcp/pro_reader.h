#ifndef DCP_PRO_READER_H
#define DCP_PRO_READER_H

#include "dcp/export.h"
#include "dcp/pro_cfg.h"
#include <stdio.h>

struct dcp_pro_model;

struct dcp_pro_reader
{
};

#if 0
DCP_API void dcp_pro_reader_init(struct dcp_pro_reader *reader,
                                 struct dcp_pro_cfg cfg, FILE *restrict fd);

DCP_API void dcp_pro_reader_next(struct dcp_pro_reader *reader,
                                 struct dcp_pro_model *model);
#endif

#endif
