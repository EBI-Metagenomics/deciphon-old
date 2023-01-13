#include "prot_h3reader.h"
#include "compiler.h"
#include "rc.h"

static void init_null_lprobs(imm_float[IMM_AMINO_SIZE]);

void prot_h3reader_init(struct prot_h3reader *reader,
                        struct imm_amino const *amino,
                        struct imm_nuclt_code const *code, struct prot_cfg cfg,
                        FILE *fp)
{
  hmr_init(&reader->hmr, fp);
  hmr_prof_init(&reader->prof, &reader->hmr);
  init_null_lprobs(reader->null_lprobs);
  prot_model_init(&reader->model, amino, code, cfg, reader->null_lprobs);
  reader->end = false;
}

int prot_h3reader_next(struct prot_h3reader *reader)
{
  int hmr_rc = hmr_next_prof(&reader->hmr, &reader->prof);
  if (hmr_rc == HMR_EOF)
  {
    reader->end = true;
    return 0;
  }

  if (hmr_rc) return EREADHMMER3;

  unsigned core_size = hmr_prof_length(&reader->prof);
  int rc = 0;
  if ((rc = prot_model_setup(&reader->model, core_size))) return rc;

  hmr_rc = hmr_next_node(&reader->hmr, &reader->prof);
  assert(hmr_rc != HMR_EOF);

  struct prot_trans t = {
      .MM = (imm_float)reader->prof.node.trans[HMR_TRANS_MM],
      .MI = (imm_float)reader->prof.node.trans[HMR_TRANS_MI],
      .MD = (imm_float)reader->prof.node.trans[HMR_TRANS_MD],
      .IM = (imm_float)reader->prof.node.trans[HMR_TRANS_IM],
      .II = (imm_float)reader->prof.node.trans[HMR_TRANS_II],
      .DM = (imm_float)reader->prof.node.trans[HMR_TRANS_DM],
      .DD = (imm_float)reader->prof.node.trans[HMR_TRANS_DD],
  };
  rc = prot_model_add_trans(&reader->model, t);
  assert(!rc);

  while (!(hmr_rc = hmr_next_node(&reader->hmr, &reader->prof)))
  {
    imm_float match_lprobs[IMM_AMINO_SIZE];
    for (unsigned i = 0; i < IMM_AMINO_SIZE; ++i)
      match_lprobs[i] = (imm_float)reader->prof.node.match[i];

    char consensus = reader->prof.node.excess.cons;
    rc = prot_model_add_node(&reader->model, match_lprobs, consensus);
    assert(!rc);

    struct prot_trans t2 = {
        .MM = (imm_float)reader->prof.node.trans[HMR_TRANS_MM],
        .MI = (imm_float)reader->prof.node.trans[HMR_TRANS_MI],
        .MD = (imm_float)reader->prof.node.trans[HMR_TRANS_MD],
        .IM = (imm_float)reader->prof.node.trans[HMR_TRANS_IM],
        .II = (imm_float)reader->prof.node.trans[HMR_TRANS_II],
        .DM = (imm_float)reader->prof.node.trans[HMR_TRANS_DM],
        .DD = (imm_float)reader->prof.node.trans[HMR_TRANS_DD],
    };
    rc = prot_model_add_trans(&reader->model, t2);
    assert(!rc);
  }
  assert(hmr_rc == HMR_END);

  return 0;
}

bool prot_h3reader_end(struct prot_h3reader const *reader)
{
  return reader->end;
}

void prot_h3reader_del(struct prot_h3reader const *reader)
{
  prot_model_del(&reader->model);
}

static void init_null_lprobs(imm_float lprobs[IMM_AMINO_SIZE])
{
  /* Copy/paste from HMMER3 amino acid frequences inferred from Swiss-Prot
   * 50.8, (Oct 2006), counting over 85956127 (86.0M) residues. */
  *(lprobs++) = imm_log(0.0787945); /*"A":*/
  *(lprobs++) = imm_log(0.0151600); /*"C":*/
  *(lprobs++) = imm_log(0.0535222); /*"D":*/
  *(lprobs++) = imm_log(0.0668298); /*"E":*/
  *(lprobs++) = imm_log(0.0397062); /*"F":*/
  *(lprobs++) = imm_log(0.0695071); /*"G":*/
  *(lprobs++) = imm_log(0.0229198); /*"H":*/
  *(lprobs++) = imm_log(0.0590092); /*"I":*/
  *(lprobs++) = imm_log(0.0594422); /*"K":*/
  *(lprobs++) = imm_log(0.0963728); /*"L":*/
  *(lprobs++) = imm_log(0.0237718); /*"M":*/
  *(lprobs++) = imm_log(0.0414386); /*"N":*/
  *(lprobs++) = imm_log(0.0482904); /*"P":*/
  *(lprobs++) = imm_log(0.0395639); /*"Q":*/
  *(lprobs++) = imm_log(0.0540978); /*"R":*/
  *(lprobs++) = imm_log(0.0683364); /*"S":*/
  *(lprobs++) = imm_log(0.0540687); /*"T":*/
  *(lprobs++) = imm_log(0.0673417); /*"V":*/
  *(lprobs++) = imm_log(0.0114135); /*"W":*/
  *(lprobs++) = imm_log(0.0304133); /*"Y":*/
};
