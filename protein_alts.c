#include "protein_alts.h"
#include "expect.h"
#include "xtrans.h"
#include <stdlib.h>

void protein_alts_init(struct protein_alts *x,
                       struct imm_nuclt_code const *code)
{
  x->core_size = 0;
  x->match_nuclt_dists = NULL;
  nuclt_dist_init(&x->insert_nuclt_dist, code->nuclt);
  imm_dp_init(&x->zero.dp, &code->super);
  imm_dp_init(&x->full.dp, &code->super);

  x->zero.S = x->full.S = 0;
  x->zero.N = x->full.N = 0;
  x->zero.B = x->full.B = 0;
  x->zero.E = x->full.E = 0;
  x->zero.J = x->full.J = 0;
  x->zero.C = x->full.C = 0;
  x->zero.T = x->full.T = 0;
}

static void alt_setup(struct protein_alt *alt, struct xtrans const *t)
{
  struct imm_dp *dp = &alt->dp;

  unsigned S = alt->S;
  unsigned N = alt->N;
  unsigned B = alt->B;
  unsigned E = alt->E;
  unsigned J = alt->J;
  unsigned C = alt->C;
  unsigned T = alt->T;

  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, S, B), t->NB);
  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, S, N), t->NN);
  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, N, N), t->NN);
  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, N, B), t->NB);

  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, E, T), t->EC + t->CT);
  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, E, C), t->EC + t->CC);
  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, C, C), t->CC);
  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, C, T), t->CT);

  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, E, B), t->EJ + t->JB);
  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, E, J), t->EJ + t->JJ);
  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, J, J), t->JJ);
  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, J, B), t->JB);
}

void protein_alts_setup(struct protein_alts *x, struct xtrans const *t)
{
  alt_setup(&x->zero, t);
  alt_setup(&x->full, t);
}

static int absorb_alt_dp(struct protein_alt *x, struct model_summary const *s)
{
  if (imm_hmm_reset_dp(s->alt.hmm, &s->alt.T->super, &x->dp))
    return DCP_EDPRESET;

  x->S = imm_state_idx(&s->alt.S->super);
  x->N = imm_state_idx(&s->alt.N->super);
  x->B = imm_state_idx(&s->alt.B->super);
  x->E = imm_state_idx(&s->alt.E->super);
  x->J = imm_state_idx(&s->alt.J->super);
  x->C = imm_state_idx(&s->alt.C->super);
  x->T = imm_state_idx(&s->alt.T->super);

  return 0;
}

static int remove_del_ins_states(struct model *x, struct model_summary const *s)
{
  int rc = 0;
  for (unsigned i = 0; i < x->core_size; ++i)
  {
    struct node *y = x->alt.nodes + i;
    if ((rc = imm_hmm_del_state(s->alt.hmm, &y->D.super))) return rc;
    if ((rc = imm_hmm_del_state(s->alt.hmm, &y->I.super))) return rc;
  }
  return rc;
}

static int alloc_match_nuclt_dists(struct protein_alts *x)
{
  size_t size = x->core_size * sizeof *x->match_nuclt_dists;
  void *ptr = realloc(x->match_nuclt_dists, size);
  if (!ptr && size > 0)
  {
    free(x->match_nuclt_dists);
    return DCP_ENOMEM;
  }
  x->match_nuclt_dists = ptr;
  return 0;
}

int protein_alts_absorb(struct protein_alts *x, struct model *m,
                        struct model_summary const *s)
{
  int rc = 0;
  x->core_size = m->core_size;
  if ((rc = alloc_match_nuclt_dists(x))) return rc;

  for (unsigned i = 0; i < x->core_size; ++i)
    x->match_nuclt_dists[i] = m->alt.nodes[i].match.nucltd;

  x->insert_nuclt_dist = m->alt.insert.nucltd;

  if ((rc = absorb_alt_dp(&x->full, s))) return rc;
  if ((rc = remove_del_ins_states(m, s))) return rc;
  if ((rc = absorb_alt_dp(&x->zero, s))) return rc;
  return rc;
}

static int pack_alt_shared(struct protein_alts const *x, struct lip_file *file)
{
  int rc = 0;
  if (!lip_write_map_size(file, 3)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "core_size")) return DCP_EFWRITE;
  if (!lip_write_int(file, x->core_size)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "insert_nuclt_dist")) return DCP_EFWRITE;
  if ((rc = nuclt_dist_pack(&x->insert_nuclt_dist, file))) return rc;

  if (!lip_write_cstr(file, "match_nuclt_dist")) return DCP_EFWRITE;
  if (!lip_write_array_size(file, x->core_size)) return DCP_EFWRITE;
  for (unsigned i = 0; i < x->core_size; ++i)
  {
    if ((rc = nuclt_dist_pack(x->match_nuclt_dists + i, file))) return rc;
  }
  return 0;
}

static int pack_alt(struct protein_alt const *x, struct lip_file *file)
{
  if (!lip_write_map_size(file, 8)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "dp")) return DCP_EFWRITE;
  if (imm_dp_pack(&x->dp, file)) return DCP_EDPPACK;

  if (!lip_write_cstr(file, "S")) return DCP_EFWRITE;
  if (!lip_write_int(file, x->S)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "N")) return DCP_EFWRITE;
  if (!lip_write_int(file, x->N)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "B")) return DCP_EFWRITE;
  if (!lip_write_int(file, x->B)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "E")) return DCP_EFWRITE;
  if (!lip_write_int(file, x->E)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "J")) return DCP_EFWRITE;
  if (!lip_write_int(file, x->J)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "C")) return DCP_EFWRITE;
  if (!lip_write_int(file, x->C)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "T")) return DCP_EFWRITE;
  if (!lip_write_int(file, x->T)) return DCP_EFWRITE;

  return 0;
}

int protein_alts_pack(struct protein_alts const *x, struct lip_file *file)
{
  int rc = 0;
  if (!lip_write_map_size(file, 3)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "shared")) return DCP_EFWRITE;
  if ((rc = pack_alt_shared(x, file))) return rc;

  if (!lip_write_cstr(file, "zero")) return DCP_EFWRITE;
  if ((rc = pack_alt(&x->zero, file))) return rc;

  if (!lip_write_cstr(file, "full")) return DCP_EFWRITE;
  if ((rc = pack_alt(&x->full, file))) return rc;
  return rc;
}

static int unpack_alt_shared(struct protein_alts *x, struct lip_file *file)
{
  int rc = 0;
  if ((rc = expect_map_size(file, 3))) return rc;

  if ((rc = expect_map_key(file, "core_size"))) return rc;
  if (!lip_read_int(file, &x->core_size)) return DCP_EFWRITE;

  if ((rc = alloc_match_nuclt_dists(x))) return rc;

  if ((rc = expect_map_key(file, "insert_nuclt_dist"))) return rc;
  if ((rc = nuclt_dist_unpack(&x->insert_nuclt_dist, file))) return rc;

  unsigned size = 0;
  if ((rc = expect_map_key(file, "match_nuclt_dist"))) return rc;
  if (!lip_read_array_size(file, &size)) return DCP_EFREAD;
  if (size != x->core_size) return DCP_EFREAD;
  for (unsigned i = 0; i < x->core_size; ++i)
  {
    if ((rc = nuclt_dist_unpack(x->match_nuclt_dists + i, file))) return rc;
    nuclt_dist_init(x->match_nuclt_dists + i,
                    x->insert_nuclt_dist.nucltp.nuclt);
  }

  return 0;
}

static int unpack_alt(struct protein_alt *x, struct lip_file *file)
{
  int rc = 0;
  if ((rc = expect_map_size(file, 8))) return rc;

  if ((rc = expect_map_key(file, "dp"))) return rc;
  if (imm_dp_unpack(&x->dp, file)) return DCP_EDPUNPACK;

  if ((rc = expect_map_key(file, "S"))) return rc;
  if (!lip_read_int(file, &x->S)) return DCP_EFREAD;

  if ((rc = expect_map_key(file, "N"))) return rc;
  if (!lip_read_int(file, &x->N)) return DCP_EFREAD;

  if ((rc = expect_map_key(file, "B"))) return rc;
  if (!lip_read_int(file, &x->B)) return DCP_EFREAD;

  if ((rc = expect_map_key(file, "E"))) return rc;
  if (!lip_read_int(file, &x->E)) return DCP_EFREAD;

  if ((rc = expect_map_key(file, "J"))) return rc;
  if (!lip_read_int(file, &x->J)) return DCP_EFREAD;

  if ((rc = expect_map_key(file, "C"))) return rc;
  if (!lip_read_int(file, &x->C)) return DCP_EFREAD;

  if ((rc = expect_map_key(file, "T"))) return rc;
  if (!lip_read_int(file, &x->T)) return DCP_EFREAD;

  return 0;
}

int protein_alts_unpack(struct protein_alts *x, struct lip_file *file)
{
  int rc = 0;
  if ((rc = expect_map_size(file, 3))) return rc;

  if ((rc = expect_map_key(file, "shared"))) return rc;
  if ((rc = unpack_alt_shared(x, file))) return rc;

  if ((rc = expect_map_key(file, "zero"))) return rc;
  if ((rc = unpack_alt(&x->zero, file))) return rc;

  if ((rc = expect_map_key(file, "full"))) return rc;
  if ((rc = unpack_alt(&x->full, file))) return rc;
  return rc;
}

void protein_alts_cleanup(struct protein_alts *x)
{
  if (x)
  {
    if (x->match_nuclt_dists) free(x->match_nuclt_dists);
    x->match_nuclt_dists = NULL;
    imm_dp_del(&x->zero.dp);
    imm_dp_del(&x->full.dp);

    x->zero.S = x->full.S = 0;
    x->zero.N = x->full.N = 0;
    x->zero.B = x->full.B = 0;
    x->zero.E = x->full.E = 0;
    x->zero.J = x->full.J = 0;
    x->zero.C = x->full.C = 0;
    x->zero.T = x->full.T = 0;
  }
}
