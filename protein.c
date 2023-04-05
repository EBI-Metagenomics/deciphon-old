#include "protein.h"
#include "db_reader.h"
#include "deciphon/errno.h"
#include "expect.h"
#include "imm/imm.h"
#include "lip/file/file.h"
#include "lip/lip.h"
#include "model.h"
#include "protein.h"
#include "strlcpy.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void protein_cleanup(struct protein *x)
{
  if (x)
  {
    x->gencode = NULL;
    if (x->alt.match_ndists) free(x->alt.match_ndists);
    x->alt.match_ndists = NULL;
    imm_dp_del(&x->null.dp);
    imm_dp_del(&x->alt.dp0);
    imm_dp_del(&x->alt.dp);
  }
}

static int alloc_match_nuclt_dists(struct protein *protein)
{
  size_t size = protein->core_size * sizeof *protein->alt.match_ndists;
  void *ptr = realloc(protein->alt.match_ndists, size);
  if (!ptr && size > 0)
  {
    free(protein->alt.match_ndists);
    return DCP_ENOMEM;
  }
  protein->alt.match_ndists = ptr;
  return 0;
}

int protein_unpack(struct protein *x, struct lip_file *file)
{
  unsigned size = 0;
  if (!lip_read_map_size(file, &size)) return DCP_EFREAD;
  assert(size == 18);

  int rc = 0;

  if ((rc = expect_map_key(file, "accession"))) return rc;
  if (!lip_read_cstr(file, ACCESSION_SIZE, x->accession)) return DCP_EFREAD;

  unsigned gencode_id = 0;
  if ((rc = expect_map_key(file, "gencode"))) return rc;
  if (!lip_read_int(file, &gencode_id)) return DCP_EFREAD;
  if (!(x->gencode = imm_gencode_get(gencode_id))) return DCP_EFREAD;

  if ((rc = expect_map_key(file, "null"))) return rc;
  if (imm_dp_unpack(&x->null.dp, file)) return DCP_EDPUNPACK;

  if ((rc = expect_map_key(file, "alt0"))) return rc;
  if (imm_dp_unpack(&x->alt.dp0, file)) return DCP_EDPUNPACK;

  if ((rc = expect_map_key(file, "alt"))) return rc;
  if (imm_dp_unpack(&x->alt.dp, file)) return DCP_EDPUNPACK;

  if ((rc = expect_map_key(file, "core_size"))) return rc;
  if (!lip_read_int(file, &size)) return DCP_EFREAD;
  if (size > MODEL_MAX) return DCP_ELARGEPROTEIN;
  x->core_size = size;

  if ((rc = expect_map_key(file, "consensus"))) return rc;
  size = x->core_size;
  if (!lip_read_cstr(file, MODEL_MAX, x->consensus)) return DCP_EFREAD;

  unsigned s = 0;

  if ((rc = expect_map_key(file, "R"))) return rc;
  if (!lip_read_int(file, &s)) return DCP_EFREAD;
  x->null.R = (unsigned)s;

  if ((rc = expect_map_key(file, "S"))) return rc;
  if (!lip_read_int(file, &s)) return DCP_EFREAD;
  x->alt.S = (unsigned)s;

  if ((rc = expect_map_key(file, "N"))) return rc;
  if (!lip_read_int(file, &s)) return DCP_EFREAD;
  x->alt.N = (unsigned)s;

  if ((rc = expect_map_key(file, "B"))) return rc;
  if (!lip_read_int(file, &s)) return DCP_EFREAD;
  x->alt.B = (unsigned)s;

  if ((rc = expect_map_key(file, "E"))) return rc;
  if (!lip_read_int(file, &s)) return DCP_EFREAD;
  x->alt.E = (unsigned)s;

  if ((rc = expect_map_key(file, "J"))) return rc;
  if (!lip_read_int(file, &s)) return DCP_EFREAD;
  x->alt.J = (unsigned)s;

  if ((rc = expect_map_key(file, "C"))) return rc;
  if (!lip_read_int(file, &s)) return DCP_EFREAD;
  x->alt.C = (unsigned)s;

  if ((rc = expect_map_key(file, "T"))) return rc;
  if (!lip_read_int(file, &s)) return DCP_EFREAD;
  x->alt.T = (unsigned)s;

  rc = alloc_match_nuclt_dists(x);
  if (rc) return rc;

  if ((rc = expect_map_key(file, "null_ndist"))) return rc;
  if ((rc = nuclt_dist_unpack(&x->null.ndist, file))) return rc;

  if ((rc = expect_map_key(file, "alt_insert_ndist"))) return rc;
  if ((rc = nuclt_dist_unpack(&x->alt.insert_ndist, file))) return rc;

  if ((rc = expect_map_key(file, "alt_match_ndist"))) return rc;
  if (!lip_read_array_size(file, &size)) return DCP_EFREAD;
  assert(size == x->core_size);
  for (unsigned i = 0; i < x->core_size; ++i)
  {
    if ((rc = nuclt_dist_unpack(x->alt.match_ndists + i, file))) return rc;
    nuclt_dist_init(x->alt.match_ndists + i, x->nuclt_code->nuclt);
  }
  return 0;
}

struct imm_dp const *protein_null_dp(struct protein const *protein)
{
  return &protein->null.dp;
}

struct imm_dp const *protein_alt0_dp(struct protein const *protein)
{
  return &protein->alt.dp0;
}

struct imm_dp const *protein_alt_dp(struct protein const *protein)
{
  return &protein->alt.dp;
}

void protein_init(struct protein *x, struct imm_gencode const *gc,
                  struct imm_amino const *amino,
                  struct imm_nuclt_code const *code, struct cfg cfg)
{
  x->gencode = gc;
  imm_strlcpy(x->accession, "", ACCESSION_SIZE);
  x->state_name = state_name;
  x->imm_code = &code->super;
  x->nuclt_code = code;
  x->amino = amino;
  x->cfg = cfg;
  x->eps = imm_frame_epsilon(cfg.eps);
  x->core_size = 0;
  x->consensus[0] = '\0';
  imm_dp_init(&x->null.dp, &code->super);
  imm_dp_init(&x->alt.dp0, &code->super);
  imm_dp_init(&x->alt.dp, &code->super);
  nuclt_dist_init(&x->null.ndist, code->nuclt);
  nuclt_dist_init(&x->alt.insert_ndist, code->nuclt);
  x->alt.match_ndists = NULL;
}

int protein_set_accession(struct protein *x, char const *acc)
{
  return imm_strlcpy(x->accession, acc, ACCESSION_SIZE) < ACCESSION_SIZE
             ? 0
             : DCP_ELONGACC;
}

int protein_setup(struct protein *protein, unsigned seq_size, bool multi_hits,
                  bool hmmer3_compat)
{
  if (seq_size == 0) return DCP_EZEROSEQ;

  float L = (float)seq_size;

  float q = 0.0;
  float log_q = IMM_LPROB_ZERO;

  if (multi_hits)
  {
    q = 0.5;
    log_q = log(0.5);
  }

  float lp = log(L) - log(L + 2 + q / (1 - q));
  float l1p = log(2 + q / (1 - q)) - log(L + 2 + q / (1 - q));
  float lr = log(L) - log(L + 1);

  struct xtrans t;

  t.NN = t.CC = t.JJ = lp;
  t.NB = t.CT = t.JB = l1p;
  t.RR = lr;
  t.EJ = log_q;
  t.EC = log(1 - q);

  if (hmmer3_compat)
  {
    t.NN = t.CC = t.JJ = log(1);
  }

  struct imm_dp *dp = &protein->null.dp;
  unsigned R = protein->null.R;
  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, R, R), t.RR);

  dp = &protein->alt.dp;
  unsigned S = protein->alt.S;
  unsigned N = protein->alt.N;
  unsigned B = protein->alt.B;
  unsigned E = protein->alt.E;
  unsigned J = protein->alt.J;
  unsigned C = protein->alt.C;
  unsigned T = protein->alt.T;

  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, S, B), t.NB);
  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, S, N), t.NN);
  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, N, N), t.NN);
  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, N, B), t.NB);

  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, E, T), t.EC + t.CT);
  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, E, C), t.EC + t.CC);
  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, C, C), t.CC);
  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, C, T), t.CT);

  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, E, B), t.EJ + t.JB);
  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, E, J), t.EJ + t.JJ);
  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, J, J), t.JJ);
  imm_dp_change_trans(dp, imm_dp_trans_idx(dp, J, B), t.JB);
  return 0;
}

int protein_absorb(struct protein *x, struct model *m)
{
  x->gencode = m->gencode;
  if (x->nuclt_code->nuclt != model_nuclt(m)) return DCP_EDIFFABC;

  if (x->amino != model_amino(m)) return DCP_EDIFFABC;

  struct model_summary s = model_summary(m);

  if (imm_hmm_reset_dp(s.null.hmm, &s.null.R->super, &x->null.dp))
    return DCP_EDPRESET;

  if (imm_hmm_reset_dp(s.alt.hmm, &s.alt.T->super, &x->alt.dp))
    return DCP_EDPRESET;

  x->core_size = m->core_size;
  memcpy(x->consensus, m->consensus, m->core_size + 1);
  int rc = alloc_match_nuclt_dists(x);
  if (rc) return rc;

  x->null.ndist = m->null.nucltd;

  for (unsigned i = 0; i < m->core_size; ++i)
    x->alt.match_ndists[i] = m->alt.nodes[i].match.nucltd;

  x->alt.insert_ndist = m->alt.insert.nucltd;

  x->null.R = imm_state_idx(&s.null.R->super);

  x->alt.S = imm_state_idx(&s.alt.S->super);
  x->alt.N = imm_state_idx(&s.alt.N->super);
  x->alt.B = imm_state_idx(&s.alt.B->super);
  x->alt.E = imm_state_idx(&s.alt.E->super);
  x->alt.J = imm_state_idx(&s.alt.J->super);
  x->alt.C = imm_state_idx(&s.alt.C->super);
  x->alt.T = imm_state_idx(&s.alt.T->super);

  for (unsigned i = 0; i < m->core_size; ++i)
  {
    rc = imm_hmm_del_state(s.alt.hmm, &m->alt.nodes[i].D.super);
    if (rc) return rc;

    rc = imm_hmm_del_state(s.alt.hmm, &m->alt.nodes[i].I.super);
    if (rc) return rc;
  }

  if (imm_hmm_reset_dp(s.alt.hmm, &s.alt.T->super, &x->alt.dp0))
    return DCP_EDPRESET;

  return 0;
}

int protein_sample(struct protein *x, unsigned seed, unsigned core_size)
{
  assert(core_size >= 2);
  if (!x->gencode) return DCP_ESETGENCODE;

  x->core_size = core_size;
  struct imm_rnd rnd = imm_rnd(seed);

  float lprobs[IMM_AMINO_SIZE];

  imm_lprob_sample(&rnd, IMM_AMINO_SIZE, lprobs);
  imm_lprob_normalize(IMM_AMINO_SIZE, lprobs);

  struct model model = {0};
  model_init(&model, x->gencode, x->amino, x->nuclt_code, x->cfg, lprobs);

  int rc = 0;

  if ((rc = model_setup(&model, core_size))) goto cleanup;

  for (unsigned i = 0; i < core_size; ++i)
  {
    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, lprobs);
    imm_lprob_normalize(IMM_AMINO_SIZE, lprobs);
    if ((rc = model_add_node(&model, lprobs, '-'))) goto cleanup;
  }

  for (unsigned i = 0; i < core_size + 1; ++i)
  {
    struct trans t;
    imm_lprob_sample(&rnd, TRANS_SIZE, t.data);
    if (i == 0) t.DD = IMM_LPROB_ZERO;
    if (i == core_size)
    {
      t.MD = IMM_LPROB_ZERO;
      t.DD = IMM_LPROB_ZERO;
    }
    imm_lprob_normalize(TRANS_SIZE, t.data);
    if ((rc = model_add_trans(&model, t))) goto cleanup;
  }

  rc = protein_absorb(x, &model);

cleanup:
  model_del(&model);
  return rc;
}

int protein_decode(struct protein const *x, struct imm_seq const *seq,
                   unsigned state_id, struct imm_codon *codon)
{
  assert(!state_is_mute(state_id));

  struct nuclt_dist const *nucltd = NULL;
  if (state_is_insert(state_id))
  {
    nucltd = &x->alt.insert_ndist;
  }
  else if (state_is_match(state_id))
  {
    unsigned idx = state_idx(state_id);
    nucltd = x->alt.match_ndists + idx;
  }
  else
    nucltd = &x->null.ndist;

  struct imm_frame_cond cond = {x->eps, &nucltd->nucltp, &nucltd->codonm};

  if (imm_lprob_is_nan(imm_frame_cond_decode(&cond, seq, codon)))
    return DCP_EDECODON;

  return 0;
}

void protein_write_dot(struct protein const *p, FILE *fp)
{
  imm_dp_write_dot(&p->alt.dp, fp, state_name);
}

void protein_write_dot0(struct protein const *p, FILE *fp)
{
  imm_dp_write_dot(&p->alt.dp0, fp, state_name);
}

int protein_pack(struct protein const *x, struct lip_file *file)
{
  if (!x->gencode) return DCP_ESETGENCODE;
  if (!lip_write_map_size(file, 18)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "accession")) return DCP_EFWRITE;
  if (!lip_write_cstr(file, x->accession)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "gencode")) return DCP_EFWRITE;
  if (!lip_write_int(file, x->gencode->id)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "null")) return DCP_EFWRITE;
  if (imm_dp_pack(&x->null.dp, file)) return DCP_EDPPACK;

  if (!lip_write_cstr(file, "alt0")) return DCP_EFWRITE;
  if (imm_dp_pack(&x->alt.dp0, file)) return DCP_EDPPACK;

  if (!lip_write_cstr(file, "alt")) return DCP_EFWRITE;
  if (imm_dp_pack(&x->alt.dp, file)) return DCP_EDPPACK;

  if (!lip_write_cstr(file, "core_size")) return DCP_EFWRITE;
  if (!lip_write_int(file, x->core_size)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "consensus")) return DCP_EFWRITE;
  if (!lip_write_cstr(file, x->consensus)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "R")) return DCP_EFWRITE;
  if (!lip_write_int(file, x->null.R)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "S")) return DCP_EFWRITE;
  if (!lip_write_int(file, x->alt.S)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "N")) return DCP_EFWRITE;
  if (!lip_write_int(file, x->alt.N)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "B")) return DCP_EFWRITE;
  if (!lip_write_int(file, x->alt.B)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "E")) return DCP_EFWRITE;
  if (!lip_write_int(file, x->alt.E)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "J")) return DCP_EFWRITE;
  if (!lip_write_int(file, x->alt.J)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "C")) return DCP_EFWRITE;
  if (!lip_write_int(file, x->alt.C)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "T")) return DCP_EFWRITE;
  if (!lip_write_int(file, x->alt.T)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "null_ndist")) return DCP_EFWRITE;
  int rc = nuclt_dist_pack(&x->null.ndist, file);
  if (rc) return rc;

  if (!lip_write_cstr(file, "alt_insert_ndist")) return DCP_EFWRITE;
  if ((rc = nuclt_dist_pack(&x->alt.insert_ndist, file))) return rc;

  if (!lip_write_cstr(file, "alt_match_ndist")) return DCP_EFWRITE;
  if (!lip_write_array_size(file, x->core_size)) return DCP_EFWRITE;
  for (unsigned i = 0; i < x->core_size; ++i)
  {
    if ((rc = nuclt_dist_pack(x->alt.match_ndists + i, file))) return rc;
  }
  return 0;
}
