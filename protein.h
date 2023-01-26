#ifndef PROTEIN_H
#define PROTEIN_H

#include "cfg.h"
#include "deciphon/errno.h"
#include "imm/imm.h"
#include "model.h"
#include <stdio.h>

enum
{
  ACCESSION_SIZE = 32,
};

struct protein
{
  char accession[ACCESSION_SIZE];
  imm_state_name *state_name;
  struct imm_code const *imm_code;

  struct imm_amino const *amino;
  struct imm_nuclt_code const *nuclt_code;
  struct cfg cfg;
  struct imm_frame_epsilon eps;
  unsigned core_size;
  char consensus[MODEL_MAX + 1];

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

struct db_reader;

void protein_init(struct protein *, struct imm_amino const *,
                  struct imm_nuclt_code const *, struct cfg);

int protein_set_accession(struct protein *, char const *);

int protein_setup(struct protein *, unsigned seq_size, bool multi_hits,
                  bool hmmer3_compat);

int protein_absorb(struct protein *, struct model const *model);

int protein_sample(struct protein *, unsigned seed, unsigned core_size);

int protein_decode(struct protein const *, struct imm_seq const *seq,
                   unsigned state_id, struct imm_codon *codon);

void protein_write_dot(struct protein const *, FILE *fp);

struct lip_file;

int protein_pack(struct protein const *, struct lip_file *file);

void protein_del(struct protein *);
int protein_unpack(struct protein *, struct lip_file *file);
struct imm_dp const *protein_null_dp(struct protein const *);
struct imm_dp const *protein_alt_dp(struct protein const *);

#endif
