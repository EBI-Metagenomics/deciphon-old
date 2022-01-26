#include "nuclt_dist.h"
#include "cmp/cmp.h"

enum rc nuclt_dist_write(struct nuclt_dist const *ndist, struct cmp_ctx_s *cmp)
{
    enum rc rc = RC_EFAIL;
    cmp_write_array(cmp, 2);
    if (imm_nuclt_lprob_write(&ndist->nucltp, cmp_file(cmp))) return rc;
    if (imm_codon_marg_write(&ndist->codonm, cmp_file(cmp))) return rc;
    return RC_DONE;
}

enum rc nuclt_dist_read(struct nuclt_dist *ndist, struct cmp_ctx_s *cmp)
{
    enum rc rc = RC_EFAIL;
    uint32_t size = 0;
    cmp_read_array(cmp, &size);
    assert(size == 2);
    if (imm_nuclt_lprob_read(&ndist->nucltp, cmp_file(cmp))) return rc;
    if (imm_codon_marg_read(&ndist->codonm, cmp_file(cmp))) return rc;
    return RC_DONE;
}
