#ifndef DCP_NUCLT_DIST_H
#define DCP_NUCLT_DIST_H

#include "dcp/export.h"
#include "imm/imm.h"

struct dcp_nuclt_dist
{
    struct imm_nuclt_lprob nucltp;
    struct imm_codon_marg codonm;
};

#endif
