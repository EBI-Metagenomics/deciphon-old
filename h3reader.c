#include "h3reader.h"
#include "compiler.h"
#include "deciphon/errno.h"

static void init_null_lprobs(float[IMM_AMINO_SIZE]);

void h3reader_init(struct h3reader *reader, struct imm_gencode const *gc,
                   struct imm_amino const *amino,
                   struct imm_nuclt_code const *code,
                   enum entry_dist entry_dist, float epsilon, FILE *fp)
{
  hmr_init(&reader->hmr, fp);
  hmr_prof_init(&reader->protein, &reader->hmr);
  init_null_lprobs(reader->null_lprobs);
  model_init(&reader->model, gc, amino, code, entry_dist, epsilon,
             reader->null_lprobs);
  reader->end = false;
}

int h3reader_next(struct h3reader *h3r)
{
  int hmr_rc = hmr_next_prof(&h3r->hmr, &h3r->protein);
  if (hmr_rc == HMR_EOF)
  {
    h3r->end = true;
    return 0;
  }

  if (hmr_rc) return DCP_EREADHMMER3;

  unsigned core_size = hmr_prof_length(&h3r->protein);
  int rc = 0;
  if ((rc = model_setup(&h3r->model, core_size))) return rc;

  hmr_rc = hmr_next_node(&h3r->hmr, &h3r->protein);
  assert(hmr_rc != HMR_EOF);

  struct trans t = {
      .MM = (float)h3r->protein.node.trans[HMR_TRANS_MM],
      .MI = (float)h3r->protein.node.trans[HMR_TRANS_MI],
      .MD = (float)h3r->protein.node.trans[HMR_TRANS_MD],
      .IM = (float)h3r->protein.node.trans[HMR_TRANS_IM],
      .II = (float)h3r->protein.node.trans[HMR_TRANS_II],
      .DM = (float)h3r->protein.node.trans[HMR_TRANS_DM],
      .DD = (float)h3r->protein.node.trans[HMR_TRANS_DD],
  };
  rc = model_add_trans(&h3r->model, t);
  assert(!rc);

  while (!(hmr_rc = hmr_next_node(&h3r->hmr, &h3r->protein)))
  {
    float match_lprobs[IMM_AMINO_SIZE];
    for (unsigned i = 0; i < IMM_AMINO_SIZE; ++i)
      match_lprobs[i] = (float)h3r->protein.node.match[i];

    char consensus = h3r->protein.node.excess.cons;
    rc = model_add_node(&h3r->model, match_lprobs, consensus);
    assert(!rc);

    struct trans t2 = {
        .MM = (float)h3r->protein.node.trans[HMR_TRANS_MM],
        .MI = (float)h3r->protein.node.trans[HMR_TRANS_MI],
        .MD = (float)h3r->protein.node.trans[HMR_TRANS_MD],
        .IM = (float)h3r->protein.node.trans[HMR_TRANS_IM],
        .II = (float)h3r->protein.node.trans[HMR_TRANS_II],
        .DM = (float)h3r->protein.node.trans[HMR_TRANS_DM],
        .DD = (float)h3r->protein.node.trans[HMR_TRANS_DD],
    };
    rc = model_add_trans(&h3r->model, t2);
    assert(!rc);
  }
  assert(hmr_rc == HMR_END);

  return 0;
}

bool h3reader_end(struct h3reader const *reader) { return reader->end; }

void h3reader_del(struct h3reader const *reader) { model_del(&reader->model); }

static void init_null_lprobs(float lprobs[IMM_AMINO_SIZE])
{
  /* Copy/paste from HMMER3 amino acid frequences inferred from Swiss-Prot
   * 50.8, (Oct 2006), counting over 85956127 (86.0M) residues. */
  *(lprobs++) = log(0.0787945); /*"A":*/
  *(lprobs++) = log(0.0151600); /*"C":*/
  *(lprobs++) = log(0.0535222); /*"D":*/
  *(lprobs++) = log(0.0668298); /*"E":*/
  *(lprobs++) = log(0.0397062); /*"F":*/
  *(lprobs++) = log(0.0695071); /*"G":*/
  *(lprobs++) = log(0.0229198); /*"H":*/
  *(lprobs++) = log(0.0590092); /*"I":*/
  *(lprobs++) = log(0.0594422); /*"K":*/
  *(lprobs++) = log(0.0963728); /*"L":*/
  *(lprobs++) = log(0.0237718); /*"M":*/
  *(lprobs++) = log(0.0414386); /*"N":*/
  *(lprobs++) = log(0.0482904); /*"P":*/
  *(lprobs++) = log(0.0395639); /*"Q":*/
  *(lprobs++) = log(0.0540978); /*"R":*/
  *(lprobs++) = log(0.0683364); /*"S":*/
  *(lprobs++) = log(0.0540687); /*"T":*/
  *(lprobs++) = log(0.0673417); /*"V":*/
  *(lprobs++) = log(0.0114135); /*"W":*/
  *(lprobs++) = log(0.0304133); /*"Y":*/
};
