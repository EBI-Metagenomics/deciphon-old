#include "nuclt_dist.h"

enum rc nuclt_dist_write(struct nuclt_dist const *ndist, struct lip_file *cmp)
{
    enum rc rc = RC_EFAIL;
    cmp_write_array(cmp, 2);
    if (imm_nuclt_lprob_write(&ndist->nucltp, cmp_file(cmp))) return rc;
    if (imm_codon_marg_write(&ndist->codonm, cmp_file(cmp))) return rc;
    return RC_DONE;
}

enum rc nuclt_dist_read(struct nuclt_dist *ndist, struct lip_file *cmp)
{
    enum rc rc = RC_EFAIL;
    uint32_t size = 0;
    cmp_read_array(cmp, &size);
    assert(size == 2);
    if (imm_nuclt_lprob_read_cmp(&ndist->nucltp, cmp)) return rc;
    if (imm_codon_marg_read_cmp(&ndist->codonm, cmp)) return rc;
    return RC_DONE;
}
