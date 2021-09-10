#ifndef DCP_NUCLT_DIST_H
#define DCP_NUCLT_DIST_H

#include "dcp/export.h"
#include "dcp/rc.h"
#include "imm/imm.h"
#include <stdio.h>

struct dcp_nuclt_dist
{
    struct imm_nuclt_lprob nucltp;
    struct imm_codon_marg codonm;
};

struct dcp_cmp;

DCP_API enum dcp_rc dcp_nuclt_dist_write(struct dcp_nuclt_dist const *ndist,
                                         struct dcp_cmp *cmp);

DCP_API enum dcp_rc dcp_nuclt_dist_read(struct dcp_nuclt_dist *ndist,
                                        struct dcp_cmp *cmp);

#endif