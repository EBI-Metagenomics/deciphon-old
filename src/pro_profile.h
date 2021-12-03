#ifndef PRO_PROF_H
#define PRO_PROF_H

#include "imm/imm.h"
#include "meta.h"
#include "pro_cfg.h"
#include "pro_model.h"
#include "profile.h"
#include "rc.h"
#include <stdio.h>

struct pro_prof
{
    struct profile super;

    struct imm_amino const *amino;
    struct imm_nuclt_code const *code;
    struct dcp_pro_cfg cfg;
    struct imm_frame_epsilon eps;
    unsigned core_size;
    char consensus[DCP_PRO_MODEL_CORE_SIZE_MAX + 1];

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

void dcp_pro_prof_init(struct pro_prof *prof, struct imm_amino const *amino,
                       struct imm_nuclt_code const *code,
                       struct dcp_pro_cfg cfg);

enum rc dcp_pro_prof_setup(struct pro_prof *prof, unsigned seq_size,
                           bool multi_hits, bool hmmer3_compat);

enum rc dcp_pro_prof_absorb(struct pro_prof *prof,
                            struct pro_model const *model);

struct profile *dcp_pro_prof_super(struct pro_prof *pro);

void dcp_pro_prof_state_name(unsigned id, char[IMM_STATE_NAME_SIZE]);

enum rc dcp_pro_prof_sample(struct pro_prof *prof, unsigned seed,
                            unsigned core_size);

enum rc dcp_pro_prof_decode(struct pro_prof const *prof,
                            struct imm_seq const *seq, unsigned state_id,
                            struct imm_codon *codon);

static inline void dcp_pro_prof_del(struct pro_prof *prof)
{
    if (prof) profile_del(&prof->super);
}

void dcp_pro_prof_write_dot(struct pro_prof const *prof, FILE *restrict fp);

struct cmp_ctx_s;
struct pro_prof;

enum rc pro_prof_read(struct pro_prof *prof, struct cmp_ctx_s *ctx);
enum rc pro_prof_write(struct pro_prof const *prof, struct cmp_ctx_s *ctx);

#endif
