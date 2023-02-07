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

void protein_del(struct protein *protein)
{
  if (protein)
  {
    if (protein->alt.match_ndists) free(protein->alt.match_ndists);
    protein->alt.match_ndists = NULL;
    imm_del(&protein->null.dp);
    imm_del(&protein->alt.dp);
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

int protein_unpack(struct protein *protein, struct lip_file *file)
{
  struct protein *p = (struct protein *)protein;
  unsigned size = 0;
  if (!lip_read_map_size(file, &size)) return DCP_EFREAD;
  assert(size == 16);

  int rc = 0;

  if ((rc = expect_map_key(file, "accession"))) return rc;
  if (!lip_read_cstr(file, ACCESSION_SIZE, protein->accession))
    return DCP_EFREAD;

  if ((rc = expect_map_key(file, "null"))) return rc;
  if (imm_dp_unpack(&p->null.dp, file)) return DCP_EDPUNPACK;

  if ((rc = expect_map_key(file, "alt"))) return rc;
  if (imm_dp_unpack(&p->alt.dp, file)) return DCP_EDPUNPACK;

  if ((rc = expect_map_key(file, "core_size"))) return rc;
  if (!lip_read_int(file, &size)) return DCP_EFREAD;
  if (size > MODEL_MAX) return DCP_ELARGEPROTEIN;
  p->core_size = size;

  if ((rc = expect_map_key(file, "consensus"))) return rc;
  size = p->core_size;
  if (!lip_read_cstr(file, MODEL_MAX, p->consensus)) return DCP_EFREAD;

  unsigned s = 0;

  if ((rc = expect_map_key(file, "R"))) return rc;
  if (!lip_read_int(file, &s)) return DCP_EFREAD;
  p->null.R = (unsigned)s;

  if ((rc = expect_map_key(file, "S"))) return rc;
  if (!lip_read_int(file, &s)) return DCP_EFREAD;
  p->alt.S = (unsigned)s;

  if ((rc = expect_map_key(file, "N"))) return rc;
  if (!lip_read_int(file, &s)) return DCP_EFREAD;
  p->alt.N = (unsigned)s;

  if ((rc = expect_map_key(file, "B"))) return rc;
  if (!lip_read_int(file, &s)) return DCP_EFREAD;
  p->alt.B = (unsigned)s;

  if ((rc = expect_map_key(file, "E"))) return rc;
  if (!lip_read_int(file, &s)) return DCP_EFREAD;
  p->alt.E = (unsigned)s;

  if ((rc = expect_map_key(file, "J"))) return rc;
  if (!lip_read_int(file, &s)) return DCP_EFREAD;
  p->alt.J = (unsigned)s;

  if ((rc = expect_map_key(file, "C"))) return rc;
  if (!lip_read_int(file, &s)) return DCP_EFREAD;
  p->alt.C = (unsigned)s;

  if ((rc = expect_map_key(file, "T"))) return rc;
  if (!lip_read_int(file, &s)) return DCP_EFREAD;
  p->alt.T = (unsigned)s;

  rc = alloc_match_nuclt_dists(p);
  if (rc) return rc;

  if ((rc = expect_map_key(file, "null_ndist"))) return rc;
  if ((rc = nuclt_dist_unpack(&p->null.ndist, file))) return rc;

  if ((rc = expect_map_key(file, "alt_insert_ndist"))) return rc;
  if ((rc = nuclt_dist_unpack(&p->alt.insert_ndist, file))) return rc;

  if ((rc = expect_map_key(file, "alt_match_ndist"))) return rc;
  if (!lip_read_array_size(file, &size)) return DCP_EFREAD;
  assert(size == p->core_size);
  for (unsigned i = 0; i < p->core_size; ++i)
  {
    if ((rc = nuclt_dist_unpack(p->alt.match_ndists + i, file))) return rc;
    nuclt_dist_init(p->alt.match_ndists + i, p->nuclt_code->nuclt);
  }
  return 0;
}

struct imm_dp const *protein_null_dp(struct protein const *protein)
{
  return &protein->null.dp;
}

struct imm_dp const *protein_alt_dp(struct protein const *protein)
{
  return &protein->alt.dp;
}

void protein_init(struct protein *p, struct imm_amino const *amino,
                  struct imm_nuclt_code const *code, struct cfg cfg)
{
  strlcpy(p->accession, "", ACCESSION_SIZE);
  p->state_name = state_name;
  p->imm_code = &code->super;
  p->nuclt_code = code;
  p->amino = amino;
  p->cfg = cfg;
  p->eps = imm_frame_epsilon(cfg.eps);
  p->core_size = 0;
  p->consensus[0] = '\0';
  imm_dp_init(&p->null.dp, &code->super);
  imm_dp_init(&p->alt.dp, &code->super);
  nuclt_dist_init(&p->null.ndist, code->nuclt);
  nuclt_dist_init(&p->alt.insert_ndist, code->nuclt);
  p->alt.match_ndists = NULL;
}

int protein_set_accession(struct protein *x, char const *acc)
{
  return strlcpy(x->accession, acc, ACCESSION_SIZE) < ACCESSION_SIZE
             ? 0
             : DCP_ELONGACC;
}

int protein_setup(struct protein *protein, unsigned seq_size, bool multi_hits,
                  bool hmmer3_compat)
{
  if (seq_size == 0) return DCP_EZEROSEQ;

  imm_float L = (imm_float)seq_size;

  imm_float q = 0.0;
  imm_float log_q = IMM_LPROB_ZERO;

  if (multi_hits)
  {
    q = 0.5;
    log_q = imm_log(0.5);
  }

  imm_float lp = imm_log(L) - imm_log(L + 2 + q / (1 - q));
  imm_float l1p = imm_log(2 + q / (1 - q)) - imm_log(L + 2 + q / (1 - q));
  imm_float lr = imm_log(L) - imm_log(L + 1);

  struct xtrans t;

  t.NN = t.CC = t.JJ = lp;
  t.NB = t.CT = t.JB = l1p;
  t.RR = lr;
  t.EJ = log_q;
  t.EC = imm_log(1 - q);

  if (hmmer3_compat)
  {
    t.NN = t.CC = t.JJ = imm_log(1);
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

int protein_absorb(struct protein *p, struct model const *m)
{
  if (p->nuclt_code->nuclt != model_nuclt(m)) return DCP_EDIFFABC;

  if (p->amino != model_amino(m)) return DCP_EDIFFABC;

  struct model_summary s = model_summary(m);

  if (imm_hmm_reset_dp(s.null.hmm, imm_super(s.null.R), &p->null.dp))
    return DCP_EDPRESET;

  if (imm_hmm_reset_dp(s.alt.hmm, imm_super(s.alt.T), &p->alt.dp))
    return DCP_EDPRESET;

  p->core_size = m->core_size;
  memcpy(p->consensus, m->consensus, m->core_size + 1);
  int rc = alloc_match_nuclt_dists(p);
  if (rc) return rc;

  p->null.ndist = m->null.nucltd;

  for (unsigned i = 0; i < m->core_size; ++i)
    p->alt.match_ndists[i] = m->alt.nodes[i].match.nucltd;

  p->alt.insert_ndist = m->alt.insert.nucltd;

  p->null.R = imm_state_idx(imm_super(s.null.R));

  p->alt.S = imm_state_idx(imm_super(s.alt.S));
  p->alt.N = imm_state_idx(imm_super(s.alt.N));
  p->alt.B = imm_state_idx(imm_super(s.alt.B));
  p->alt.E = imm_state_idx(imm_super(s.alt.E));
  p->alt.J = imm_state_idx(imm_super(s.alt.J));
  p->alt.C = imm_state_idx(imm_super(s.alt.C));
  p->alt.T = imm_state_idx(imm_super(s.alt.T));
  return 0;
}

int protein_sample(struct protein *p, unsigned seed, unsigned core_size)
{
  assert(core_size >= 2);
  p->core_size = core_size;
  struct imm_rnd rnd = imm_rnd(seed);

  imm_float lprobs[IMM_AMINO_SIZE];

  imm_lprob_sample(&rnd, IMM_AMINO_SIZE, lprobs);
  imm_lprob_normalize(IMM_AMINO_SIZE, lprobs);

  struct model model;
  model_init(&model, p->amino, p->nuclt_code, p->cfg, lprobs);

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

  rc = protein_absorb(p, &model);

cleanup:
  model_del(&model);
  return rc;
}

int protein_decode(struct protein const *protein, struct imm_seq const *seq,
                   unsigned state_id, struct imm_codon *codon)
{
  assert(!state_is_mute(state_id));

  struct nuclt_dist const *nucltd = NULL;
  if (state_is_insert(state_id))
  {
    nucltd = &protein->alt.insert_ndist;
  }
  else if (state_is_match(state_id))
  {
    unsigned idx = state_idx(state_id);
    nucltd = protein->alt.match_ndists + idx;
  }
  else
    nucltd = &protein->null.ndist;

  struct imm_frame_cond cond = {protein->eps, &nucltd->nucltp, &nucltd->codonm};

  if (imm_lprob_is_nan(imm_frame_cond_decode(&cond, seq, codon)))
    return DCP_EDECODON;

  return 0;
}

void protein_write_dot(struct protein const *p, FILE *fp)
{
  imm_dp_write_dot(&p->alt.dp, fp, state_name);
}

int protein_pack(struct protein const *protein, struct lip_file *file)
{
  if (!lip_write_map_size(file, 16)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "accession")) return DCP_EFWRITE;
  if (!lip_write_cstr(file, protein->accession)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "null")) return DCP_EFWRITE;
  if (imm_dp_pack(&protein->null.dp, file)) return DCP_EDPPACK;

  if (!lip_write_cstr(file, "alt")) return DCP_EFWRITE;
  if (imm_dp_pack(&protein->alt.dp, file)) return DCP_EDPPACK;

  if (!lip_write_cstr(file, "core_size")) return DCP_EFWRITE;
  if (!lip_write_int(file, protein->core_size)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "consensus")) return DCP_EFWRITE;
  if (!lip_write_cstr(file, protein->consensus)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "R")) return DCP_EFWRITE;
  if (!lip_write_int(file, protein->null.R)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "S")) return DCP_EFWRITE;
  if (!lip_write_int(file, protein->alt.S)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "N")) return DCP_EFWRITE;
  if (!lip_write_int(file, protein->alt.N)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "B")) return DCP_EFWRITE;
  if (!lip_write_int(file, protein->alt.B)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "E")) return DCP_EFWRITE;
  if (!lip_write_int(file, protein->alt.E)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "J")) return DCP_EFWRITE;
  if (!lip_write_int(file, protein->alt.J)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "C")) return DCP_EFWRITE;
  if (!lip_write_int(file, protein->alt.C)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "T")) return DCP_EFWRITE;
  if (!lip_write_int(file, protein->alt.T)) return DCP_EFWRITE;

  if (!lip_write_cstr(file, "null_ndist")) return DCP_EFWRITE;
  int rc = nuclt_dist_pack(&protein->null.ndist, file);
  if (rc) return rc;

  if (!lip_write_cstr(file, "alt_insert_ndist")) return DCP_EFWRITE;
  if ((rc = nuclt_dist_pack(&protein->alt.insert_ndist, file))) return rc;

  if (!lip_write_cstr(file, "alt_match_ndist")) return DCP_EFWRITE;
  if (!lip_write_array_size(file, protein->core_size)) return DCP_EFWRITE;
  for (unsigned i = 0; i < protein->core_size; ++i)
  {
    if ((rc = nuclt_dist_pack(protein->alt.match_ndists + i, file))) return rc;
  }
  return 0;
}
