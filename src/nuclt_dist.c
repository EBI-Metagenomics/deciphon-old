#include "dcp/nuclt_dist.h"
#include "dcp/cmp.h"

enum dcp_rc dcp_nuclt_dist_write(struct dcp_nuclt_dist const *ndist,
                                 struct dcp_cmp *cmp)
{
    enum dcp_rc rc = DCP_RUNTIMEERROR;
    if (imm_nuclt_lprob_write(&ndist->nucltp, dcp_cmp_fd(cmp))) return rc;
    if (imm_codon_marg_write(&ndist->codonm, dcp_cmp_fd(cmp))) return rc;
    return DCP_SUCCESS;
}

enum dcp_rc dcp_nuclt_dist_read(struct dcp_nuclt_dist *ndist,
                                struct dcp_cmp *cmp)
{
    enum dcp_rc rc = DCP_RUNTIMEERROR;
    if (imm_nuclt_lprob_read(&ndist->nucltp, dcp_cmp_fd(cmp))) return rc;
    if (imm_codon_marg_read(&ndist->codonm, dcp_cmp_fd(cmp))) return rc;
    return DCP_SUCCESS;
}