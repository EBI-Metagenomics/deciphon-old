#ifndef PROTEIN_CODEC_H
#define PROTEIN_CODEC_H

#include "rc.h"

struct protein_prof;
struct imm_codon;
struct imm_path;
struct imm_seq;

struct dcp_protein_codec
{
    unsigned idx;
    unsigned start;
    struct protein_prof const *prof;
    struct imm_path const *path;
};

static inline struct dcp_protein_codec
dcp_protein_codec_init(struct protein_prof const *prof, struct imm_path const *path)
{
    return (struct dcp_protein_codec){0, 0, prof, path};
}

enum rc dcp_protein_codec_next(struct dcp_protein_codec *codec,
                               struct imm_seq const *seq,
                               struct imm_codon *codon);

#endif
