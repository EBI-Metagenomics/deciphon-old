#ifndef NUCLT_DIST_H
#define NUCLT_DIST_H

#include "imm/imm.h"
#include "rc.h"
#include <stdio.h>

struct dcp_nuclt_dist
{
    struct imm_nuclt_lprob nucltp;
    struct imm_codon_marg codonm;
};

struct cmp_ctx_s;

static inline void dcp_nuclt_dist_init(struct dcp_nuclt_dist *nucltd,
                                       struct imm_nuclt const *nuclt)
{
    nucltd->nucltp.nuclt = nuclt;
    nucltd->codonm.nuclt = nuclt;
}

enum dcp_rc dcp_nuclt_dist_write(struct dcp_nuclt_dist const *ndist,
                                 struct cmp_ctx_s *cmp);

enum dcp_rc dcp_nuclt_dist_read(struct dcp_nuclt_dist *ndist,
                                struct cmp_ctx_s *cmp);

#endif
