#include "nuclt_dist.h"
#include "dcp_cmp.h"
#include "xcmp.h"

enum dcp_rc dcp_nuclt_dist_write(struct dcp_nuclt_dist const *ndist,
                                 struct cmp_ctx_s *cmp)
{
    enum dcp_rc rc = DCP_FAIL;
    if (imm_nuclt_lprob_write(&ndist->nucltp, xcmp_fp(cmp))) return rc;
    if (imm_codon_marg_write(&ndist->codonm, xcmp_fp(cmp))) return rc;
    return DCP_DONE;
}

enum dcp_rc dcp_nuclt_dist_read(struct dcp_nuclt_dist *ndist,
                                struct cmp_ctx_s *cmp)
{
    enum dcp_rc rc = DCP_FAIL;
    if (imm_nuclt_lprob_read(&ndist->nucltp, xcmp_fp(cmp))) return rc;
    if (imm_codon_marg_read(&ndist->codonm, xcmp_fp(cmp))) return rc;
    return DCP_DONE;
}
