#ifndef PROT_PROF_H
#define PROT_PROF_H

#include "deciphon_limits.h"
#include "imm/imm.h"
#include "prof.h"
#include "prot_cfg.h"
#include "prot_model.h"
#include "rc.h"
#include <stdio.h>

struct prot_prof
{
  struct prof super;

  struct imm_amino const *amino;
  struct imm_nuclt_code const *code;
  struct prot_cfg cfg;
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

void prot_prof_init(struct prot_prof *prof, char const *accession,
                    struct imm_amino const *amino,
                    struct imm_nuclt_code const *code, struct prot_cfg cfg);

int prot_prof_setup(struct prot_prof *prof, unsigned seq_size, bool multi_hits,
                    bool hmmer3_compat);

int prot_prof_absorb(struct prot_prof *prof, struct prot_model const *model);

int prot_prof_sample(struct prot_prof *prof, unsigned seed, unsigned core_size);

int prot_prof_decode(struct prot_prof const *prof, struct imm_seq const *seq,
                     unsigned state_id, struct imm_codon *codon);

void prot_prof_write_dot(struct prot_prof const *prof, FILE *fp);

struct lip_file;

int prot_prof_unpack(struct prot_prof *prof, struct lip_file *file);
int prot_prof_pack(struct prot_prof const *prof, struct lip_file *file);

#endif
