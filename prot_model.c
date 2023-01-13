#include "prot_model.h"
#include "entry_dist.h"
#include "nuclt_dist.h"
#include "prot_model.h"
#include "prot_node.h"
#include "rc.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define LOGN2 (imm_float)(-1.3862943611198906) /* log(1./4.) */
#define LOG1 IMM_LPROB_ONE

static imm_float const uniform_lprobs[IMM_NUCLT_SIZE] = {LOGN2, LOGN2, LOGN2,
                                                         LOGN2};

/* Compute log(1 - p) given log(p). */
static inline imm_float log1_p(imm_float logp) { return log1p(-exp(logp)); }

int add_xnodes(struct prot_model *);
void init_xnodes(struct prot_model *);

void calculate_occupancy(struct prot_model *);

bool have_called_setup(struct prot_model *);
bool have_finished_add(struct prot_model const *);

void init_delete(struct imm_mute_state *, struct prot_model *);
void init_insert(struct imm_frame_state *, struct prot_model *);
void init_match(struct imm_frame_state *, struct prot_model *,
                struct nuclt_dist *);

int init_null_xtrans(struct imm_hmm *, struct prot_xnode_null *);
int init_alt_xtrans(struct imm_hmm *, struct prot_xnode_alt *);

struct imm_nuclt_lprob nuclt_lprob(struct imm_codon_lprob const *);
struct imm_codon_lprob codon_lprob(struct imm_amino const *,
                                   struct imm_amino_lprob const *,
                                   struct imm_nuclt const *);

void setup_nuclt_dist(struct nuclt_dist *, struct imm_amino const *,
                      struct imm_nuclt const *,
                      imm_float const[IMM_AMINO_SIZE]);

int setup_entry_trans(struct prot_model *);
int setup_exit_trans(struct prot_model *);
int setup_transitions(struct prot_model *);

int prot_model_add_node(struct prot_model *m,
                        imm_float const lprobs[IMM_AMINO_SIZE], char consensus)
{
  if (!have_called_setup(m)) return EFUNCUSE;

  if (m->alt.node_idx == m->core_size) return ELARGEMODEL;

  m->consensus[m->alt.node_idx] = consensus;

  imm_float lodds[IMM_AMINO_SIZE];
  for (unsigned i = 0; i < IMM_AMINO_SIZE; ++i)
    lodds[i] = lprobs[i] - m->null.lprobs[i];

  struct prot_node *n = m->alt.nodes + m->alt.node_idx;

  setup_nuclt_dist(&n->match.nucltd, m->amino, m->code->nuclt, lodds);

  init_match(&n->M, m, &n->match.nucltd);
  if (imm_hmm_add_state(&m->alt.hmm, &n->M.super)) return EADDSTATE;

  init_insert(&n->I, m);
  if (imm_hmm_add_state(&m->alt.hmm, &n->I.super)) return EADDSTATE;

  init_delete(&n->D, m);
  if (imm_hmm_add_state(&m->alt.hmm, &n->D.super)) return EADDSTATE;

  m->alt.node_idx++;

  if (have_finished_add(m)) setup_transitions(m);

  return 0;
}

int prot_model_add_trans(struct prot_model *m, struct prot_trans trans)
{
  if (!have_called_setup(m)) return EFUNCUSE;

  if (m->alt.trans_idx == m->core_size + 1) return EMANYTRANS;

  m->alt.trans[m->alt.trans_idx++] = trans;
  if (have_finished_add(m)) setup_transitions(m);
  return 0;
}

void prot_model_del(struct prot_model const *model)
{
  free(model->alt.nodes);
  free(model->alt.locc);
  free(model->alt.trans);
}

void prot_model_init(struct prot_model *m, struct imm_amino const *amino,
                     struct imm_nuclt_code const *code, struct prot_cfg cfg,
                     imm_float const null_lprobs[IMM_AMINO_SIZE])

{
  m->amino = amino;
  m->code = code;
  m->cfg = cfg;
  m->core_size = 0;
  m->consensus[0] = '\0';
  struct imm_nuclt const *nuclt = code->nuclt;

  memcpy(m->null.lprobs, null_lprobs, sizeof *null_lprobs * IMM_AMINO_SIZE);

  imm_hmm_init(&m->null.hmm, &m->code->super);

  setup_nuclt_dist(&m->null.nucltd, amino, nuclt, null_lprobs);

  imm_hmm_init(&m->alt.hmm, &m->code->super);

  imm_float const lodds[IMM_AMINO_SIZE] = {0};
  setup_nuclt_dist(&m->alt.insert.nucltd, amino, nuclt, lodds);

  init_xnodes(m);

  m->alt.node_idx = UINT_MAX;
  m->alt.nodes = NULL;
  m->alt.locc = NULL;
  m->alt.trans_idx = UINT_MAX;
  m->alt.trans = NULL;
  prot_xtrans_init(&m->xtrans);
}

static void model_reset(struct prot_model *model)
{
  imm_hmm_reset(&model->null.hmm);
  imm_hmm_reset(&model->alt.hmm);

  imm_state_detach(&model->xnode.null.R.super);

  imm_state_detach(&model->xnode.alt.S.super);
  imm_state_detach(&model->xnode.alt.N.super);
  imm_state_detach(&model->xnode.alt.B.super);
  imm_state_detach(&model->xnode.alt.E.super);
  imm_state_detach(&model->xnode.alt.J.super);
  imm_state_detach(&model->xnode.alt.C.super);
  imm_state_detach(&model->xnode.alt.T.super);
}

int prot_model_setup(struct prot_model *m, unsigned core_size)
{
  if (core_size == 0) return EINVAL;

  if (core_size > PROT_MODEL_CORE_SIZE_MAX) return ELARGEMODEL;

  m->core_size = core_size;
  m->consensus[core_size] = '\0';
  unsigned n = m->core_size;
  m->alt.node_idx = 0;

  void *ptr = realloc(m->alt.nodes, n * sizeof(*m->alt.nodes));
  if (!ptr && n > 0) return ENOMEM;
  m->alt.nodes = ptr;

  if (m->cfg.edist == ENTRY_DIST_OCCUPANCY)
  {
    ptr = realloc(m->alt.locc, n * sizeof(*m->alt.locc));
    if (!ptr && n > 0) return ENOMEM;
    m->alt.locc = ptr;
  }
  m->alt.trans_idx = 0;
  ptr = realloc(m->alt.trans, (n + 1) * sizeof(*m->alt.trans));
  if (!ptr) return ENOMEM;
  m->alt.trans = ptr;

  model_reset(m);
  return add_xnodes(m);
}

void prot_model_write_dot(struct prot_model const *m, FILE *fp)
{
  imm_hmm_write_dot(&m->alt.hmm, fp, prot_state_name);
}

struct imm_amino const *prot_model_amino(struct prot_model const *m)
{
  return m->amino;
}

struct imm_nuclt const *prot_model_nuclt(struct prot_model const *m)
{
  return m->code->nuclt;
}

struct prot_model_summary prot_model_summary(struct prot_model const *m)
{
  assert(have_finished_add(m));
  return (struct prot_model_summary){
      .null = {.hmm = &m->null.hmm, .R = &m->xnode.null.R},
      .alt = {
          .hmm = &m->alt.hmm,
          .S = &m->xnode.alt.S,
          .N = &m->xnode.alt.N,
          .B = &m->xnode.alt.B,
          .E = &m->xnode.alt.E,
          .J = &m->xnode.alt.J,
          .C = &m->xnode.alt.C,
          .T = &m->xnode.alt.T,
      }};
}

int add_xnodes(struct prot_model *m)
{
  struct prot_xnode *n = &m->xnode;

  if (imm_hmm_add_state(&m->null.hmm, &n->null.R.super)) return EADDSTATE;
  if (imm_hmm_set_start(&m->null.hmm, &n->null.R.super, LOG1)) return ESETTRANS;

  if (imm_hmm_add_state(&m->alt.hmm, &n->alt.S.super)) return EADDSTATE;
  if (imm_hmm_add_state(&m->alt.hmm, &n->alt.N.super)) return EADDSTATE;
  if (imm_hmm_add_state(&m->alt.hmm, &n->alt.B.super)) return EADDSTATE;
  if (imm_hmm_add_state(&m->alt.hmm, &n->alt.E.super)) return EADDSTATE;
  if (imm_hmm_add_state(&m->alt.hmm, &n->alt.J.super)) return EADDSTATE;
  if (imm_hmm_add_state(&m->alt.hmm, &n->alt.C.super)) return EADDSTATE;
  if (imm_hmm_add_state(&m->alt.hmm, &n->alt.T.super)) return EADDSTATE;
  if (imm_hmm_set_start(&m->alt.hmm, &n->alt.S.super, LOG1)) return ESETTRANS;

  return 0;
}

void init_xnodes(struct prot_model *m)
{
  imm_float e = m->cfg.eps;
  struct imm_nuclt_lprob const *nucltp = &m->null.nucltd.nucltp;
  struct imm_codon_marg const *codonm = &m->null.nucltd.codonm;
  struct prot_xnode *n = &m->xnode;
  struct imm_nuclt const *nuclt = m->code->nuclt;

  struct imm_span w = imm_span(1, 5);
  imm_frame_state_init(&n->null.R, PROT_R_STATE, nucltp, codonm, e, w);

  imm_mute_state_init(&n->alt.S, PROT_S_STATE, &nuclt->super);
  imm_frame_state_init(&n->alt.N, PROT_N_STATE, nucltp, codonm, e, w);
  imm_mute_state_init(&n->alt.B, PROT_B_STATE, &nuclt->super);
  imm_mute_state_init(&n->alt.E, PROT_E_STATE, &nuclt->super);
  imm_frame_state_init(&n->alt.J, PROT_J_STATE, nucltp, codonm, e, w);
  imm_frame_state_init(&n->alt.C, PROT_C_STATE, nucltp, codonm, e, w);
  imm_mute_state_init(&n->alt.T, PROT_T_STATE, &nuclt->super);
}

void calculate_occupancy(struct prot_model *m)
{
  struct prot_trans *trans = m->alt.trans;
  m->alt.locc[0] = imm_lprob_add(trans->MI, trans->MM);
  for (unsigned i = 1; i < m->core_size; ++i)
  {
    ++trans;
    imm_float v0 = m->alt.locc[i - 1] + imm_lprob_add(trans->MM, trans->MI);
    imm_float v1 = log1_p(m->alt.locc[i - 1]) + trans->DM;
    m->alt.locc[i] = imm_lprob_add(v0, v1);
  }

  imm_float logZ = imm_lprob_zero();
  unsigned n = m->core_size;
  for (unsigned i = 0; i < m->core_size; ++i)
  {
    logZ = imm_lprob_add(logZ, m->alt.locc[i] + imm_log(n - i));
  }

  for (unsigned i = 0; i < m->core_size; ++i)
  {
    m->alt.locc[i] -= logZ;
  }

  assert(!imm_lprob_is_nan(logZ));
}

bool have_called_setup(struct prot_model *m) { return m->core_size > 0; }

bool have_finished_add(struct prot_model const *m)
{
  unsigned core_size = m->core_size;
  return m->alt.node_idx == core_size && m->alt.trans_idx == (core_size + 1);
}

void init_delete(struct imm_mute_state *state, struct prot_model *m)
{
  unsigned id = PROT_DELETE_STATE | (m->alt.node_idx + 1);
  imm_mute_state_init(state, id, &m->code->nuclt->super);
}

void init_insert(struct imm_frame_state *state, struct prot_model *m)
{
  imm_float e = m->cfg.eps;
  unsigned id = PROT_INSERT_STATE | (m->alt.node_idx + 1);
  struct imm_nuclt_lprob *nucltp = &m->alt.insert.nucltd.nucltp;
  struct imm_codon_marg *codonm = &m->alt.insert.nucltd.codonm;

  imm_frame_state_init(state, id, nucltp, codonm, e, imm_span(1, 5));
}

void init_match(struct imm_frame_state *state, struct prot_model *m,
                struct nuclt_dist *d)
{
  imm_float e = m->cfg.eps;
  unsigned id = PROT_MATCH_STATE | (m->alt.node_idx + 1);
  imm_frame_state_init(state, id, &d->nucltp, &d->codonm, e, imm_span(1, 5));
}

int init_null_xtrans(struct imm_hmm *hmm, struct prot_xnode_null *n)
{
  if (imm_hmm_set_trans(hmm, &n->R.super, &n->R.super, LOG1)) return ESETTRANS;
  return 0;
}

int init_alt_xtrans(struct imm_hmm *hmm, struct prot_xnode_alt *n)
{
  if (imm_hmm_set_trans(hmm, &n->S.super, &n->B.super, LOG1)) return ESETTRANS;
  if (imm_hmm_set_trans(hmm, &n->S.super, &n->N.super, LOG1)) return ESETTRANS;
  if (imm_hmm_set_trans(hmm, &n->N.super, &n->N.super, LOG1)) return ESETTRANS;
  if (imm_hmm_set_trans(hmm, &n->N.super, &n->B.super, LOG1)) return ESETTRANS;

  if (imm_hmm_set_trans(hmm, &n->E.super, &n->T.super, LOG1)) return ESETTRANS;
  if (imm_hmm_set_trans(hmm, &n->E.super, &n->C.super, LOG1)) return ESETTRANS;
  if (imm_hmm_set_trans(hmm, &n->C.super, &n->C.super, LOG1)) return ESETTRANS;
  if (imm_hmm_set_trans(hmm, &n->C.super, &n->T.super, LOG1)) return ESETTRANS;

  if (imm_hmm_set_trans(hmm, &n->E.super, &n->B.super, LOG1)) return ESETTRANS;
  if (imm_hmm_set_trans(hmm, &n->E.super, &n->J.super, LOG1)) return ESETTRANS;
  if (imm_hmm_set_trans(hmm, &n->J.super, &n->J.super, LOG1)) return ESETTRANS;
  if (imm_hmm_set_trans(hmm, &n->J.super, &n->B.super, LOG1)) return ESETTRANS;

  return 0;
}

struct imm_nuclt_lprob nuclt_lprob(struct imm_codon_lprob const *codonp)
{
  imm_float lprobs[] = {[0 ... IMM_NUCLT_SIZE - 1] = IMM_LPROB_ZERO};

  imm_float const norm = imm_log((imm_float)3);
  for (unsigned i = 0; i < imm_gc_size(); ++i)
  {
    struct imm_codon codon = imm_gc_codon(1, i);
    /* Check for FIXME-1 for an explanation of this
     * temporary hacky */
    codon.nuclt = codonp->nuclt;
    imm_float lprob = imm_codon_lprob_get(codonp, codon);
    lprobs[codon.a] = imm_lprob_add(lprobs[codon.a], lprob - norm);
    lprobs[codon.b] = imm_lprob_add(lprobs[codon.b], lprob - norm);
    lprobs[codon.c] = imm_lprob_add(lprobs[codon.c], lprob - norm);
  }
  return imm_nuclt_lprob(codonp->nuclt, lprobs);
}

struct imm_codon_lprob codon_lprob(struct imm_amino const *amino,
                                   struct imm_amino_lprob const *aminop,
                                   struct imm_nuclt const *nuclt)
{
  /* FIXME: We don't need 255 positions*/
  unsigned count[] = {[0 ... 254] = 0};

  for (unsigned i = 0; i < imm_gc_size(); ++i)
    count[(unsigned)imm_gc_aa(1, i)] += 1;

  struct imm_abc const *abc = &amino->super;
  /* TODO: We don't need 255 positions*/
  imm_float lprobs[] = {[0 ... 254] = IMM_LPROB_ZERO};
  for (unsigned i = 0; i < imm_abc_size(abc); ++i)
  {
    char aa = imm_abc_symbols(abc)[i];
    imm_float norm = imm_log((imm_float)count[(unsigned)aa]);
    lprobs[(unsigned)aa] = imm_amino_lprob_get(aminop, aa) - norm;
  }

  /* FIXME-1: imm_gc module assumes imm_dna_iupac as alphabet, we have to make
   * it configurable. For now I will assume that the calle of this
   * function is using imm_nuclt base of an imm_dna_iupac compatible alphabet
   */
  /* struct imm_codon_lprob codonp = imm_codon_lprob(nuclt); */
  struct imm_codon_lprob codonp = imm_codon_lprob(&imm_gc_dna->super);
  for (unsigned i = 0; i < imm_gc_size(); ++i)
  {
    char aa = imm_gc_aa(1, i);
    imm_codon_lprob_set(&codonp, imm_gc_codon(1, i), lprobs[(unsigned)aa]);
  }
  codonp.nuclt = nuclt;
  return codonp;
}

void setup_nuclt_dist(struct nuclt_dist *dist, struct imm_amino const *amino,
                      struct imm_nuclt const *nuclt,
                      imm_float const lprobs[IMM_AMINO_SIZE])
{
  dist->nucltp = imm_nuclt_lprob(nuclt, uniform_lprobs);
  struct imm_amino_lprob aminop = imm_amino_lprob(amino, lprobs);
  struct imm_codon_lprob codonp =
      codon_lprob(amino, &aminop, dist->nucltp.nuclt);
  imm_codon_lprob_normalize(&codonp);

  dist->nucltp = nuclt_lprob(&codonp);
  dist->codonm = imm_codon_marg(&codonp);
}

int setup_entry_trans(struct prot_model *m)
{
  if (m->cfg.edist == ENTRY_DIST_UNIFORM)
  {
    imm_float M = (imm_float)m->core_size;
    imm_float cost = imm_log(2.0 / (M * (M + 1))) * M;

    struct imm_state *B = &m->xnode.alt.B.super;
    for (unsigned i = 0; i < m->core_size; ++i)
    {
      struct prot_node *node = m->alt.nodes + i;
      if (imm_hmm_set_trans(&m->alt.hmm, B, &node->M.super, cost))
        return ESETTRANS;
    }
  }
  else
  {
    assert(m->cfg.edist == ENTRY_DIST_OCCUPANCY);
    calculate_occupancy(m);
    struct imm_state *B = &m->xnode.alt.B.super;
    for (unsigned i = 0; i < m->core_size; ++i)
    {
      struct prot_node *node = m->alt.nodes + i;
      if (imm_hmm_set_trans(&m->alt.hmm, B, &node->M.super, m->alt.locc[i]))
        return ESETTRANS;
    }
  }
  return 0;
}

int setup_exit_trans(struct prot_model *m)
{
  struct imm_state *E = &m->xnode.alt.E.super;

  for (unsigned i = 0; i < m->core_size; ++i)
  {
    struct prot_node *node = m->alt.nodes + i;
    if (imm_hmm_set_trans(&m->alt.hmm, &node->M.super, E, imm_log(1)))
      return ESETTRANS;
  }
  for (unsigned i = 1; i < m->core_size; ++i)
  {
    struct prot_node *node = m->alt.nodes + i;
    if (imm_hmm_set_trans(&m->alt.hmm, &node->D.super, E, imm_log(1)))
      return ESETTRANS;
  }
  return 0;
}

int setup_transitions(struct prot_model *m)
{
  struct imm_hmm *h = &m->alt.hmm;
  struct prot_trans *trans = m->alt.trans;

  struct imm_state *B = &m->xnode.alt.B.super;
  struct imm_state *M1 = &m->alt.nodes[0].M.super;
  if (imm_hmm_set_trans(h, B, M1, trans[0].MM)) return ESETTRANS;

  for (unsigned i = 0; i + 1 < m->core_size; ++i)
  {
    struct prot_node *p = m->alt.nodes + i;
    struct prot_node *n = m->alt.nodes + i + 1;
    unsigned j = i + 1;
    struct prot_trans t = trans[j];
    if (imm_hmm_set_trans(h, &p->M.super, &p->I.super, t.MI)) return ESETTRANS;
    if (imm_hmm_set_trans(h, &p->I.super, &p->I.super, t.II)) return ESETTRANS;
    if (imm_hmm_set_trans(h, &p->M.super, &n->M.super, t.MM)) return ESETTRANS;
    if (imm_hmm_set_trans(h, &p->I.super, &n->M.super, t.IM)) return ESETTRANS;
    if (imm_hmm_set_trans(h, &p->M.super, &n->D.super, t.MD)) return ESETTRANS;
    if (imm_hmm_set_trans(h, &p->D.super, &n->D.super, t.DD)) return ESETTRANS;
    if (imm_hmm_set_trans(h, &p->D.super, &n->M.super, t.DM)) return ESETTRANS;
  }

  unsigned n = m->core_size;
  struct imm_state *Mm = &m->alt.nodes[n - 1].M.super;
  struct imm_state *E = &m->xnode.alt.E.super;
  if (imm_hmm_set_trans(h, Mm, E, trans[n].MM)) return ESETTRANS;

  if (setup_entry_trans(m)) return ESETTRANS;
  if (setup_exit_trans(m)) return ESETTRANS;
  if (init_null_xtrans(&m->null.hmm, &m->xnode.null)) return ESETTRANS;
  return init_alt_xtrans(&m->alt.hmm, &m->xnode.alt);
}
