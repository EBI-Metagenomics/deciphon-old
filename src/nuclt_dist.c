#include "nuclt_dist.h"
#include "dcp_cmp.h"
#include "xcmp.h"

enum rc nuclt_dist_write(struct nuclt_dist const *ndist,
                                 struct cmp_ctx_s *cmp)
{
    enum rc rc = RC_FAIL;
    if (imm_nuclt_lprob_write(&ndist->nucltp, xcmp_fp(cmp))) return rc;
    if (imm_codon_marg_write(&ndist->codonm, xcmp_fp(cmp))) return rc;
    return RC_DONE;
}

enum rc nuclt_dist_read(struct nuclt_dist *ndist,
                                struct cmp_ctx_s *cmp)
{
    enum rc rc = RC_FAIL;
    if (imm_nuclt_lprob_read(&ndist->nucltp, xcmp_fp(cmp))) return rc;
    if (imm_codon_marg_read(&ndist->codonm, xcmp_fp(cmp))) return rc;
    return RC_DONE;
}
