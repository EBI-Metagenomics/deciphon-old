#ifndef PRO_CODEC_H
#define PRO_CODEC_H

#include "rc.h"

struct dcp_pro_prof;
struct imm_codon;
struct imm_path;
struct imm_seq;

struct dcp_pro_codec
{
    unsigned idx;
    unsigned start;
    struct dcp_pro_prof const *prof;
    struct imm_path const *path;
};

static inline struct dcp_pro_codec
dcp_pro_codec_init(struct dcp_pro_prof const *prof, struct imm_path const *path)
{
    return (struct dcp_pro_codec){0, 0, prof, path};
}

enum dcp_rc dcp_pro_codec_next(struct dcp_pro_codec *codec,
                               struct imm_seq const *seq,
                               struct imm_codon *codon);

#endif
