#ifndef CODEC_H
#define CODEC_H

#include "deciphon/errno.h"
#include <stdbool.h>

struct imm_codon;
struct imm_path;
struct imm_seq;
struct protein;

struct codec
{
  unsigned idx;
  unsigned start;
  struct protein const *protein;
  struct imm_path const *path;
};

struct codec codec_init(struct protein const *, struct imm_path const *);
int codec_next(struct codec *, struct imm_seq const *, struct imm_codon *);
bool codec_end(struct codec const *);

#endif
