#include "protein_profile.h"
#include "hope/hope.h"
#include "imm/imm.h"
#include "protein_codec.h"

void test_protein_profile_uniform(void);
void test_protein_profile_occupancy(void);

int main(void)
{
    test_protein_profile_uniform();
    test_protein_profile_occupancy();
    return hope_status();
}

void test_protein_profile_uniform(void)
{
    struct imm_amino const *amino = &imm_amino_iupac;
    struct imm_nuclt const *nuclt = imm_super(&imm_dna_iupac);
    struct imm_nuclt_code code;
    imm_nuclt_code_init(&code, nuclt);
    struct protein_cfg cfg = protein_cfg(DCP_ENTRY_DIST_UNIFORM, 0.1f);

    struct protein_profile prof;
    protein_profile_init(&prof, amino, &code, cfg);
    EQ(protein_profile_sample(&prof, 1, 2), DONE);

    char const str[] = "ATGAAACGCATTAGCACCACCATTACCACCAC";
    struct imm_seq seq = imm_seq(IMM_STR(str), prof.super.code->abc);

    EQ(protein_profile_setup(&prof, 0, true, false), ILLEGALARG);
    EQ(protein_profile_setup(&prof, imm_seq_size(&seq), true, false), DONE);

    struct imm_prod prod = imm_prod();
    struct imm_dp *dp = &prof.null.dp;
    struct imm_task *task = imm_task_new(dp);
    NOTNULL(task);
    EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
    EQ(imm_dp_viterbi(dp, task, &prod), IMM_SUCCESS);

    CLOSE(prod.loglik, -48.9272687711);

    EQ(imm_path_nsteps(&prod.path), 11);
    char name[IMM_STATE_NAME_SIZE];

    EQ(imm_path_step(&prod.path, 0)->seqlen, 3);
    EQ(imm_path_step(&prod.path, 0)->state_id, PROTEIN_ID_R);
    protein_profile_state_name(imm_path_step(&prod.path, 0)->state_id, name);
    EQ(name, "R");

    EQ(imm_path_step(&prod.path, 10)->seqlen, 2);
    EQ(imm_path_step(&prod.path, 10)->state_id, PROTEIN_ID_R);
    protein_profile_state_name(imm_path_step(&prod.path, 10)->state_id, name);
    EQ(name, "R");

    imm_prod_reset(&prod);
    imm_del(task);

    dp = &prof.alt.dp;
    task = imm_task_new(dp);
    NOTNULL(task);
    EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
    EQ(imm_dp_viterbi(dp, task, &prod), IMM_SUCCESS);

    CLOSE(prod.loglik, -55.59428153448);

    EQ(imm_path_nsteps(&prod.path), 14);

    EQ(imm_path_step(&prod.path, 0)->seqlen, 0);
    EQ(imm_path_step(&prod.path, 0)->state_id, PROTEIN_ID_S);
    protein_profile_state_name(imm_path_step(&prod.path, 0)->state_id, name);
    EQ(name, "S");

    EQ(imm_path_step(&prod.path, 13)->seqlen, 0);
    EQ(imm_path_step(&prod.path, 13)->state_id, PROTEIN_ID_T);
    protein_profile_state_name(imm_path_step(&prod.path, 13)->state_id, name);
    EQ(name, "T");

    struct protein_codec codec = protein_codec_init(&prof, &prod.path);
    enum rc rc = DONE;

    nuclt = prof.code->nuclt;
    struct imm_codon codons[10] = {
        IMM_CODON(nuclt, "ATG"), IMM_CODON(nuclt, "AAA"),
        IMM_CODON(nuclt, "CGC"), IMM_CODON(nuclt, "ATA"),
        IMM_CODON(nuclt, "GCA"), IMM_CODON(nuclt, "CCA"),
        IMM_CODON(nuclt, "CCT"), IMM_CODON(nuclt, "TAC"),
        IMM_CODON(nuclt, "CAC"), IMM_CODON(nuclt, "CAC"),
    };

    unsigned any = imm_abc_any_symbol_id(imm_super(nuclt));
    struct imm_codon codon = imm_codon(nuclt, any, any, any);
    unsigned i = 0;
    while (!(rc = protein_codec_next(&codec, &seq, &codon)))
    {
        EQ(codons[i].a, codon.a);
        EQ(codons[i].b, codon.b);
        EQ(codons[i].c, codon.c);
        ++i;
    }
    EQ(rc, END);
    EQ(i, 10);

    protein_profile_del(&prof);
    imm_del(&prod);
    imm_del(task);
}

void test_protein_profile_occupancy(void)
{
    struct imm_amino const *amino = &imm_amino_iupac;
    struct imm_nuclt const *nuclt = imm_super(&imm_dna_iupac);
    struct imm_nuclt_code code;
    imm_nuclt_code_init(&code, nuclt);
    struct protein_cfg cfg = protein_cfg(DCP_ENTRY_DIST_OCCUPANCY, 0.1f);

    struct protein_profile prof;
    protein_profile_init(&prof, amino, &code, cfg);
    EQ(protein_profile_sample(&prof, 1, 2), DONE);

    char const str[] = "ATGAAACGCATTAGCACCACCATTACCACCAC";
    struct imm_seq seq = imm_seq(imm_str(str), prof.super.code->abc);

    EQ(protein_profile_setup(&prof, imm_seq_size(&seq), true, false), DONE);

    struct imm_prod prod = imm_prod();
    struct imm_dp *dp = &prof.null.dp;
    struct imm_task *task = imm_task_new(dp);
    NOTNULL(task);
    EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
    EQ(imm_dp_viterbi(dp, task, &prod), IMM_SUCCESS);

    CLOSE(prod.loglik, -48.9272687711);

    EQ(imm_path_nsteps(&prod.path), 11);
    char name[IMM_STATE_NAME_SIZE];

    EQ(imm_path_step(&prod.path, 0)->seqlen, 3);
    EQ(imm_path_step(&prod.path, 0)->state_id, PROTEIN_ID_R);
    protein_profile_state_name(imm_path_step(&prod.path, 0)->state_id, name);
    EQ(name, "R");

    EQ(imm_path_step(&prod.path, 10)->seqlen, 2);
    EQ(imm_path_step(&prod.path, 10)->state_id, PROTEIN_ID_R);
    protein_profile_state_name(imm_path_step(&prod.path, 10)->state_id, name);
    EQ(name, "R");

    imm_prod_reset(&prod);
    imm_del(task);

    dp = &prof.alt.dp;
    task = imm_task_new(dp);
    NOTNULL(task);
    EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
    EQ(imm_dp_viterbi(dp, task, &prod), IMM_SUCCESS);

    CLOSE(prod.loglik, -54.35543421312);

    EQ(imm_path_nsteps(&prod.path), 14);

    EQ(imm_path_step(&prod.path, 0)->seqlen, 0);
    EQ(imm_path_step(&prod.path, 0)->state_id, PROTEIN_ID_S);
    protein_profile_state_name(imm_path_step(&prod.path, 0)->state_id, name);
    EQ(name, "S");

    EQ(imm_path_step(&prod.path, 13)->seqlen, 0);
    EQ(imm_path_step(&prod.path, 13)->state_id, PROTEIN_ID_T);
    protein_profile_state_name(imm_path_step(&prod.path, 13)->state_id, name);
    EQ(name, "T");

    struct protein_codec codec = protein_codec_init(&prof, &prod.path);
    enum rc rc = DONE;

    nuclt = prof.code->nuclt;
    struct imm_codon codons[10] = {
        IMM_CODON(nuclt, "ATG"), IMM_CODON(nuclt, "AAA"),
        IMM_CODON(nuclt, "CGC"), IMM_CODON(nuclt, "ATA"),
        IMM_CODON(nuclt, "GCA"), IMM_CODON(nuclt, "CCA"),
        IMM_CODON(nuclt, "CCT"), IMM_CODON(nuclt, "TAC"),
        IMM_CODON(nuclt, "CAC"), IMM_CODON(nuclt, "CAC"),
    };

    unsigned any = imm_abc_any_symbol_id(imm_super(nuclt));
    struct imm_codon codon = imm_codon(nuclt, any, any, any);
    unsigned i = 0;
    while (!(rc = protein_codec_next(&codec, &seq, &codon)))
    {
        EQ(codons[i].a, codon.a);
        EQ(codons[i].b, codon.b);
        EQ(codons[i].c, codon.c);
        ++i;
    }
    EQ(rc, END);
    EQ(i, 10);

    protein_profile_del(&prof);
    imm_del(&prod);
    imm_del(task);
}
