#ifndef MODEL_H
#define MODEL_H

#include "cfg.h"
#include "deciphon_limits.h"
#include "entry_dist.h"
#include "imm/imm.h"
#include "node.h"
#include "nuclt_dist.h"
#include "rc.h"
#include "state.h"
#include "trans.h"
#include "xnode.h"
#include "xtrans.h"

struct model
{
  struct imm_amino const *amino;
  struct imm_nuclt_code const *code;
  struct cfg cfg;
  unsigned core_size;
  struct prot_xnode xnode;
  struct prot_xtrans xtrans;
  char consensus[PROT_MODEL_CORE_SIZE_MAX + 1];

  struct
  {
    imm_float lprobs[IMM_AMINO_SIZE];
    struct nuclt_dist nucltd;
    struct imm_hmm hmm;
  } null;

  struct
  {
    unsigned node_idx;
    struct node *nodes;
    imm_float *locc;
    unsigned trans_idx;
    struct trans *trans;
    struct imm_hmm hmm;

    struct
    {
      struct nuclt_dist nucltd;
    } insert;
  } alt;
};

int model_add_node(struct model *, imm_float const lp[IMM_AMINO_SIZE],
                   char consensus);

int model_add_trans(struct model *, struct trans trans);

void model_del(struct model const *);

void model_init(struct model *, struct imm_amino const *amino,
                struct imm_nuclt_code const *code, struct cfg cfg,
                imm_float const null_lprobs[IMM_AMINO_SIZE]);

int model_setup(struct model *, unsigned core_size);

void model_write_dot(struct model const *, FILE *fp);

struct prot_model_summary
{
  struct
  {
    struct imm_hmm const *hmm;
    struct imm_frame_state const *R;
  } null;

  struct
  {
    struct imm_hmm const *hmm;
    struct imm_mute_state const *S;
    struct imm_frame_state const *N;
    struct imm_mute_state const *B;
    struct imm_mute_state const *E;
    struct imm_frame_state const *J;
    struct imm_frame_state const *C;
    struct imm_mute_state const *T;
  } alt;
};

struct model;

struct imm_amino const *prot_model_amino(struct model const *m);
struct imm_nuclt const *prot_model_nuclt(struct model const *m);
struct prot_model_summary prot_model_summary(struct model const *m);

#endif
