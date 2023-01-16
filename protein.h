#ifndef PROTEIN_H
#define PROTEIN_H

#include "cfg.h"
#include "deciphon_limits.h"
#include "imm/imm.h"
#include "model.h"
#include "rc.h"
#include <stdio.h>

struct protein
{
  char acc[PROF_ACC_SIZE];
  imm_state_name *state_name;
  struct imm_code const *imm_code;

  struct imm_amino const *amino;
  struct imm_nuclt_code const *nuclt_code;
  struct cfg cfg;
  struct imm_frame_epsilon eps;
  unsigned core_size;
  char consensus[PROT_MODEL_CORE_SIZE_MAX + 1];

  struct
  {
    struct nuclt_dist ndist;
    struct imm_dp dp;
    unsigned R;
  } null;

  struct
  {
    struct nuclt_dist *match_ndists;
    struct nuclt_dist insert_ndist;
    struct imm_dp dp;
    unsigned S;
    unsigned N;
    unsigned B;
    unsigned E;
    unsigned J;
    unsigned C;
    unsigned T;
  } alt;
};

void protein_init(struct protein *prof, char const *accession,
                  struct imm_amino const *amino,
                  struct imm_nuclt_code const *code, struct cfg cfg);

int protein_setup(struct protein *prof, unsigned seq_size, bool multi_hits,
                  bool hmmer3_compat);

int protein_absorb(struct protein *prof, struct model const *model);

int protein_sample(struct protein *prof, unsigned seed, unsigned core_size);

int protein_decode(struct protein const *prof, struct imm_seq const *seq,
                   unsigned state_id, struct imm_codon *codon);

void protein_write_dot(struct protein const *prof, FILE *fp);

struct lip_file;

int protein_pack(struct protein const *prof, struct lip_file *file);

void protein_del(struct protein *prof);
int protein_unpack(struct protein *prof, struct lip_file *file);
struct imm_dp const *protein_null_dp(struct protein const *prof);
struct imm_dp const *protein_alt_dp(struct protein const *prof);

#endif
