#ifndef DCP_PP_H
#define DCP_PP_H

#include "dcp/export.h"
#include "imm/imm.h"

struct dcp_pp;

DCP_API struct dcp_pp *
dcp_pp_create(imm_float const null_lprobs[IMM_AMINO_SIZE]);

DCP_API int dcp_pp_add_node(struct dcp_pp *pp, imm_float lodds[static 1]);

DCP_API void dcp_pp_destroy(struct dcp_pp *pp);

#endif
