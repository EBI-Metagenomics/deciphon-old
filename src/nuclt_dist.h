#ifndef NUCLT_DIST_H
#define NUCLT_DIST_H

#include "imm/imm.h"
#include "rc.h"
#include <stdio.h>

struct nuclt_dist
{
    struct imm_nuclt_lprob nucltp;
    struct imm_codon_marg codonm;
};

struct cmp_ctx_s;

static inline void nuclt_dist_init(struct nuclt_dist *nucltd,
                                   struct imm_nuclt const *nuclt)
{
    nucltd->nucltp.nuclt = nuclt;
    nucltd->codonm.nuclt = nuclt;
}

enum rc nuclt_dist_write(struct nuclt_dist const *ndist, struct cmp_ctx_s *cmp);

enum rc nuclt_dist_read(struct nuclt_dist *ndist, struct cmp_ctx_s *cmp);

#endif
