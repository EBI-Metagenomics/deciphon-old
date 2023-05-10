#include "protein_null.h"
#include "expect.h"
#include "xtrans.h"

void protein_null_init(struct protein_null *x,
                       struct imm_nuclt_code const *code)
{
  nuclt_dist_init(&x->nuclt_dist, code->nuclt);
  imm_dp_init(&x->dp, &code->super);
  x->R = 0;
}

void protein_null_setup(struct protein_null *x, struct xtrans const *t)
{
  imm_dp_change_trans(&x->dp, imm_dp_trans_idx(&x->dp, x->R, x->R), t->RR);
}

int protein_null_absorb(struct protein_null *x, struct model const *m,
                        struct model_summary const *s)
{
  struct imm_hmm const *hmm = s->null.hmm;
  if (imm_hmm_reset_dp(hmm, &s->null.R->super, &x->dp)) return DCP_EDPRESET;
  x->nuclt_dist = m->null.nuclt_dist;
  x->R = imm_state_idx(&s->null.R->super);
  return 0;
}

int protein_null_pack(struct protein_null const *x, struct lip_file *file)
{
  int rc = 0;
  if (!lip_write_map_size(file, 3)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "dp")) return DCP_EFWRITE;
  if (imm_dp_pack(&x->dp, file)) return DCP_EDPPACK;

  if (!lip_write_cstr(file, "nuclt_dist")) return DCP_EFWRITE;
  if ((rc = nuclt_dist_pack(&x->nuclt_dist, file))) return rc;

  if (!lip_write_cstr(file, "R")) return DCP_EFWRITE;
  if (!lip_write_int(file, x->R)) return DCP_EFWRITE;
  return 0;
}

int protein_null_unpack(struct protein_null *x, struct lip_file *file)
{
  int rc = 0;
  if ((rc = expect_map_size(file, 3))) return rc;

  if ((rc = expect_map_key(file, "dp"))) return rc;
  if (imm_dp_unpack(&x->dp, file)) return DCP_EDPUNPACK;

  if ((rc = expect_map_key(file, "nuclt_dist"))) return rc;
  if ((rc = nuclt_dist_unpack(&x->nuclt_dist, file))) return rc;

  if ((rc = expect_map_key(file, "R"))) return rc;
  if (!lip_read_int(file, &x->R)) return DCP_EFREAD;
  return 0;
}

void protein_null_cleanup(struct protein_null *x)
{
  if (x) imm_dp_del(&x->dp);
}
