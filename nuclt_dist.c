#include "nuclt_dist.h"
#include "lite_pack/lite_pack.h"

void nuclt_dist_init(struct nuclt_dist *nucltd, struct imm_nuclt const *nuclt)
{
  nucltd->nucltp.nuclt = nuclt;
  nucltd->codonm.nuclt = nuclt;
}

int nuclt_dist_pack(struct nuclt_dist const *ndist, struct lip_file *file)
{
  int rc = ENUCLTDPACK;
  lip_write_array_size(file, 2);
  if (imm_nuclt_lprob_pack(&ndist->nucltp, file)) return rc;
  if (imm_codon_marg_pack(&ndist->codonm, file)) return rc;
  return 0;
}

int nuclt_dist_unpack(struct nuclt_dist *ndist, struct lip_file *file)
{
  int rc = ENUCLTDUNPACK;
  unsigned size = 0;
  lip_read_array_size(file, &size);
  assert(size == 2);
  if (imm_nuclt_lprob_unpack(&ndist->nucltp, file)) return rc;
  if (imm_codon_marg_unpack(&ndist->codonm, file)) return rc;
  return 0;
}
