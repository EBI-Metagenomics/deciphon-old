#ifndef MODEL_PROT_CODEC_H
#define MODEL_PROT_CODEC_H

#include "rc.h"
#include <stdbool.h>

struct imm_codon;
struct imm_path;
struct imm_seq;
struct prot_prof;

struct prot_codec
{
  unsigned idx;
  unsigned start;
  struct prot_prof const *prof;
  struct imm_path const *path;
};

struct prot_codec prot_codec_init(struct prot_prof const *prof,
                                  struct imm_path const *path);

int prot_codec_next(struct prot_codec *codec, struct imm_seq const *seq,
                    struct imm_codon *codon);

bool prot_codec_end(struct prot_codec const *);

#endif
