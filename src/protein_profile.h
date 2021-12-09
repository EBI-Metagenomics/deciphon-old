#ifndef PROTEIN_PROFILE_H
#define PROTEIN_PROFILE_H

#include "dcp_limits.h"
#include "imm/imm.h"
#include "metadata.h"
#include "profile.h"
#include "protein_cfg.h"
#include "protein_model.h"
#include "rc.h"
#include <stdio.h>

struct protein_profile
{
    struct profile super;

    struct imm_amino const *amino;
    struct imm_nuclt_code const *code;
    struct protein_cfg cfg;
    struct imm_frame_epsilon eps;
    unsigned core_size;
    char consensus[PROTEIN_MODEL_CORE_SIZE_MAX + 1];

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

void protein_profile_init(struct protein_profile *prof,
                          struct imm_amino const *amino,
                          struct imm_nuclt_code const *code,
                          struct protein_cfg cfg);

enum rc protein_profile_setup(struct protein_profile *prof, unsigned seq_size,
                              bool multi_hits, bool hmmer3_compat);

enum rc protein_profile_absorb(struct protein_profile *prof,
                               struct protein_model const *model);

unsigned protein_profile_state_name(unsigned id, char[IMM_STATE_NAME_SIZE]);

enum rc protein_profile_sample(struct protein_profile *prof, unsigned seed,
                               unsigned core_size);

enum rc protein_profile_decode(struct protein_profile const *prof,
                               struct imm_seq const *seq, unsigned state_id,
                               struct imm_codon *codon);

void protein_profile_write_dot(struct protein_profile const *prof,
                               FILE *restrict fp);

struct cmp_ctx_s;

enum rc protein_profile_read(struct protein_profile *prof,
                             struct cmp_ctx_s *ctx);
enum rc protein_profile_write(struct protein_profile const *prof,
                              struct cmp_ctx_s *ctx);

#endif
