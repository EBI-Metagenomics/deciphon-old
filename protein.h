#ifndef PROTEIN_H
#define PROTEIN_H

#include "imm/imm.h"
#include "model.h"
#include "protein_alts.h"
#include "protein_null.h"

struct lip_file;
struct db_reader;

struct protein
{
  struct imm_gencode const *gencode;
  char accession[32];
  imm_state_name *state_name;
  struct imm_code const *imm_code;

  struct imm_amino const *amino;
  struct imm_nuclt_code const *nuclt_code;
  enum entry_dist entry_dist;
  float epsilon;
  struct imm_frame_epsilon epsilon_frame;
  char consensus[MODEL_MAX + 1];

  struct protein_null null;
  struct protein_alts alts;
};

void protein_init(struct protein *, struct imm_gencode const *,
                  struct imm_amino const *, struct imm_nuclt_code const *,
                  enum entry_dist, float epsilon);

int protein_set_accession(struct protein *, char const *);

void protein_setup(struct protein *, unsigned seq_size, bool multi_hits,
                   bool hmmer3_compat);

int protein_absorb(struct protein *, struct model *model);

int protein_sample(struct protein *, unsigned seed, unsigned core_size);

int protein_decode(struct protein const *, struct imm_seq const *,
                   unsigned state_id, struct imm_codon *codon);

void protein_write_dot(struct protein const *, struct imm_dp const *, FILE *);

int protein_pack(struct protein const *, struct lip_file *file);
int protein_unpack(struct protein *, struct lip_file *file);

void protein_cleanup(struct protein *);

#endif
