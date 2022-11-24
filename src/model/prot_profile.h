#ifndef MODEL_PROT_PROFILE_H
#define MODEL_PROT_PROFILE_H

#include "deciphon_limits.h"
#include "imm/imm.h"
#include "model/profile.h"
#include "model/prot_cfg.h"
#include "model/prot_model.h"
#include "rc.h"
#include <stdio.h>

struct prot_profile
{
    struct profile super;

    struct imm_amino const *amino;
    struct imm_nuclt_code const *code;
    struct prot_cfg cfg;
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

void prot_profile_init(struct prot_profile *prof, char const *accession,
                       struct imm_amino const *amino,
                       struct imm_nuclt_code const *code, struct prot_cfg cfg);

enum rc prot_profile_setup(struct prot_profile *prof, unsigned seq_size,
                           bool multi_hits, bool hmmer3_compat);

enum rc prot_profile_absorb(struct prot_profile *prof,
                            struct prot_model const *model);

enum rc prot_profile_sample(struct prot_profile *prof, unsigned seed,
                            unsigned core_size);

enum rc prot_profile_decode(struct prot_profile const *prof,
                            struct imm_seq const *seq, unsigned state_id,
                            struct imm_codon *codon);

void prot_profile_write_dot(struct prot_profile const *prof, FILE *fp);

struct lip_file;

enum rc prot_profile_unpack(struct prot_profile *prof, struct lip_file *file);
enum rc prot_profile_pack(struct prot_profile const *prof,
                          struct lip_file *file);

#endif
