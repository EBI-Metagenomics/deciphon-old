#ifndef MODEL_PROT_CODEC_H
#define MODEL_PROT_CODEC_H

#include "rc.h"

struct imm_codon;
struct imm_path;
struct imm_seq;
struct prot_profile;

struct prot_codec
{
    unsigned idx;
    unsigned start;
    struct prot_profile const *prof;
    struct imm_path const *path;
};

static inline struct prot_codec prot_codec_init(struct prot_profile const *prof,
                                                struct imm_path const *path)
{
    return (struct prot_codec){0, 0, prof, path};
}

enum rc prot_codec_next(struct prot_codec *codec, struct imm_seq const *seq,
                        struct imm_codon *codon);

#endif
