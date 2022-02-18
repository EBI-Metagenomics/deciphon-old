#ifndef NUCLT_DIST_H
#define NUCLT_DIST_H

#include "imm/imm.h"
#include "common/rc.h"
#include <stdio.h>

struct nuclt_dist
{
    struct imm_nuclt_lprob nucltp;
    struct imm_codon_marg codonm;
};

struct lip_io_file;

static inline void nuclt_dist_init(struct nuclt_dist *nucltd,
                                   struct imm_nuclt const *nuclt)
{
    nucltd->nucltp.nuclt = nuclt;
    nucltd->codonm.nuclt = nuclt;
}

enum rc nuclt_dist_write(struct nuclt_dist const *ndist, struct lip_io_file *cmp);

enum rc nuclt_dist_read(struct nuclt_dist *ndist, struct lip_io_file *cmp);

#endif
