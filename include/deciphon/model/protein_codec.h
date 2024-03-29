#ifndef DECIPHON_MODEL_PROTEIN_CODEC_H
#define DECIPHON_MODEL_PROTEIN_CODEC_H

#include "deciphon/core/rc.h"

struct imm_codon;
struct imm_path;
struct imm_seq;
struct protein_profile;

struct protein_codec
{
    unsigned idx;
    unsigned start;
    struct protein_profile const *prof;
    struct imm_path const *path;
};

static inline struct protein_codec
protein_codec_init(struct protein_profile const *prof,
                   struct imm_path const *path)
{
    return (struct protein_codec){0, 0, prof, path};
}

enum rc protein_codec_next(struct protein_codec *codec,
                           struct imm_seq const *seq, struct imm_codon *codon);

#endif
