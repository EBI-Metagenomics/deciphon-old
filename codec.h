#ifndef CODEC_H
#define CODEC_H

#include "rc.h"
#include <stdbool.h>

struct imm_codon;
struct imm_path;
struct imm_seq;
struct protein;

struct codec
{
  unsigned idx;
  unsigned start;
  struct protein const *prof;
  struct imm_path const *path;
};

struct codec codec_init(struct protein const *prof,
                        struct imm_path const *path);

int codec_next(struct codec *codec, struct imm_seq const *seq,
               struct imm_codon *codon);

bool codec_end(struct codec const *);

#endif
