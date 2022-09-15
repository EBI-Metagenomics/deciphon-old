#include "model/nuclt_dist.h"
#include "lite_pack/file/file.h"
#include "lite_pack/lite_pack.h"

enum rc nuclt_dist_pack(struct nuclt_dist const *ndist, struct lip_file *file)
{
    enum rc rc = RC_EFAIL;
    lip_write_array_size(file, 2);
    if (imm_nuclt_lprob_pack(&ndist->nucltp, file)) return rc;
    if (imm_codon_marg_pack(&ndist->codonm, file)) return rc;
    return RC_OK;
}

enum rc nuclt_dist_unpack(struct nuclt_dist *ndist, struct lip_file *file)
{
    enum rc rc = RC_EFAIL;
    unsigned size = 0;
    lip_read_array_size(file, &size);
    assert(size == 2);
    if (imm_nuclt_lprob_unpack(&ndist->nucltp, file)) return rc;
    if (imm_codon_marg_unpack(&ndist->codonm, file)) return rc;
    return RC_OK;
}
