#ifndef PROTEIN_PROF_H
#define PROTEIN_PROF_H

#include "imm/imm.h"
#include "meta.h"
#include "protein_cfg.h"
#include "protein_model.h"
#include "profile.h"
#include "rc.h"
#include <stdio.h>

struct protein_prof
{
    struct profile super;

    struct imm_amino const *amino;
    struct imm_nuclt_code const *code;
    struct dcp_protein_cfg cfg;
    struct imm_frame_epsilon eps;
    unsigned core_size;
    char consensus[DCP_PROTEIN_MODEL_CORE_SIZE_MAX + 1];

    struct
    {
        struct dcp_nuclt_dist ndist;
        struct imm_dp dp;
        unsigned R;
    } null;

    struct
    {
        struct dcp_nuclt_dist *match_ndists;
        struct dcp_nuclt_dist insert_ndist;
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

void dcp_protein_prof_init(struct protein_prof *prof, struct imm_amino const *amino,
                       struct imm_nuclt_code const *code,
                       struct dcp_protein_cfg cfg);

enum rc dcp_protein_prof_setup(struct protein_prof *prof, unsigned seq_size,
                           bool multi_hits, bool hmmer3_compat);

enum rc dcp_protein_prof_absorb(struct protein_prof *prof,
                            struct protein_model const *model);

struct profile *dcp_protein_prof_super(struct protein_prof *pro);

void dcp_protein_prof_state_name(unsigned id, char[IMM_STATE_NAME_SIZE]);

enum rc dcp_protein_prof_sample(struct protein_prof *prof, unsigned seed,
                            unsigned core_size);

enum rc dcp_protein_prof_decode(struct protein_prof const *prof,
                            struct imm_seq const *seq, unsigned state_id,
                            struct imm_codon *codon);

static inline void dcp_protein_prof_del(struct protein_prof *prof)
{
    if (prof) profile_del(&prof->super);
}

void dcp_protein_prof_write_dot(struct protein_prof const *prof, FILE *restrict fp);

struct cmp_ctx_s;
struct protein_prof;

enum rc protein_prof_read(struct protein_prof *prof, struct cmp_ctx_s *ctx);
enum rc protein_prof_write(struct protein_prof const *prof, struct cmp_ctx_s *ctx);

#endif
