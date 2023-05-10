#ifndef MODEL_H
#define MODEL_H

#include "deciphon/errno.h"
#include "entry_dist.h"
#include "imm/imm.h"
#include "model_summary.h"
#include "node.h"
#include "nuclt_dist.h"
#include "state.h"
#include "trans.h"
#include "xnode.h"
#include "xtrans.h"

enum
{
  MODEL_MAX = 4096,
};

struct model
{
  struct imm_gencode const *gencode;
  struct imm_amino const *amino;
  struct imm_nuclt_code const *code;
  enum entry_dist entry_dist;
  float epsilon;
  unsigned core_size;
  struct xnode xnode;
  struct xtrans xtrans;
  char consensus[MODEL_MAX + 1];

  struct
  {
    float lprobs[IMM_AMINO_SIZE];
    struct nuclt_dist nuclt_dist;
    struct imm_hmm hmm;
  } null;

  struct
  {
    unsigned node_idx;
    struct node *nodes;
    float *locc;
    unsigned trans_idx;
    struct trans *trans;
    struct imm_hmm hmm;

    struct
    {
      struct nuclt_dist nucltd;
    } insert;
  } alt;
};

int model_add_node(struct model *, float const lp[IMM_AMINO_SIZE],
                   char consensus);

int model_add_trans(struct model *, struct trans);

void model_del(struct model const *);

void model_init(struct model *, struct imm_gencode const *,
                struct imm_amino const *, struct imm_nuclt_code const *,
                enum entry_dist, float epsilon,
                float const null_lprobs[IMM_AMINO_SIZE]);

int model_setup(struct model *, unsigned core_size);

void model_write_dot(struct model const *, FILE *);

struct imm_amino const *model_amino(struct model const *);
struct imm_nuclt const *model_nuclt(struct model const *);
struct model_summary model_summary(struct model *);

#endif
