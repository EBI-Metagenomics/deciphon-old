#ifndef DECIPHON_MODEL_NUCLT_DIST_H
#define DECIPHON_MODEL_NUCLT_DIST_H

#include "imm/imm.h"
#include "rc.h"

struct nuclt_dist
{
    struct imm_nuclt_lprob nucltp;
    struct imm_codon_marg codonm;
};

struct lip_file;

static inline void nuclt_dist_init(struct nuclt_dist *nucltd,
                                   struct imm_nuclt const *nuclt)
{
    nucltd->nucltp.nuclt = nuclt;
    nucltd->codonm.nuclt = nuclt;
}

enum rc nuclt_dist_pack(struct nuclt_dist const *ndist, struct lip_file *);

enum rc nuclt_dist_unpack(struct nuclt_dist *ndist, struct lip_file *);

#endif
