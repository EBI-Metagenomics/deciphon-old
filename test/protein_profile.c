#include "hope.h"
#include "imm/imm.h"
#include "model/prot_codec.h"
#include "model/prot_profile.h"

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
    struct prot_cfg cfg = prot_cfg(ENTRY_DIST_UNIFORM, 0.1f);

    struct prot_profile prof;
    prot_profile_init(&prof, "accession", amino, &code, cfg);
    eq(prot_profile_sample(&prof, 1, 2), RC_OK);

    char const str[] = "ATGAAACGCATTAGCACCACCATTACCACCAC";
    struct imm_seq seq = imm_seq(IMM_STR(str), prof.super.code->abc);

    eq(prot_profile_setup(&prof, 0, true, false), RC_EINVAL);
    eq(prot_profile_setup(&prof, imm_seq_size(&seq), true, false), RC_OK);

    struct imm_prod prod = imm_prod();
    struct imm_dp *dp = &prof.null.dp;
    struct imm_task *task = imm_task_new(dp);
    notnull(task);
    eq(imm_task_setup(task, &seq), IMM_OK);
    eq(imm_dp_viterbi(dp, task, &prod), IMM_OK);

    close(prod.loglik, -48.9272687711);

    eq(imm_path_nsteps(&prod.path), 11);
    char name[IMM_STATE_NAME_SIZE];

    eq(imm_path_step(&prod.path, 0)->seqlen, 3);
    eq(imm_path_step(&prod.path, 0)->state_id, PROTEIN_R_STATE);
    prot_state_name(imm_path_step(&prod.path, 0)->state_id, name);
    eq(name, "R");

    eq(imm_path_step(&prod.path, 10)->seqlen, 2);
    eq(imm_path_step(&prod.path, 10)->state_id, PROTEIN_R_STATE);
    prot_state_name(imm_path_step(&prod.path, 10)->state_id, name);
    eq(name, "R");

    imm_prod_reset(&prod);
    imm_del(task);

    dp = &prof.alt.dp;
    task = imm_task_new(dp);
    notnull(task);
    eq(imm_task_setup(task, &seq), IMM_OK);
    eq(imm_dp_viterbi(dp, task, &prod), IMM_OK);

    close(prod.loglik, -55.59428153448);

    eq(imm_path_nsteps(&prod.path), 14);

    eq(imm_path_step(&prod.path, 0)->seqlen, 0);
    eq(imm_path_step(&prod.path, 0)->state_id, PROTEIN_S_STATE);
    prot_state_name(imm_path_step(&prod.path, 0)->state_id, name);
    eq(name, "S");

    eq(imm_path_step(&prod.path, 13)->seqlen, 0);
    eq(imm_path_step(&prod.path, 13)->state_id, PROTEIN_T_STATE);
    prot_state_name(imm_path_step(&prod.path, 13)->state_id, name);
    eq(name, "T");

    struct prot_codec codec = prot_codec_init(&prof, &prod.path);
    enum rc rc = RC_OK;

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
    while (!(rc = prot_codec_next(&codec, &seq, &codon)))
    {
        eq(codons[i].a, codon.a);
        eq(codons[i].b, codon.b);
        eq(codons[i].c, codon.c);
        ++i;
    }
    eq(rc, RC_END);
    eq(i, 10);

    profile_del((struct profile *)&prof);
    imm_del(&prod);
    imm_del(task);
}

void test_protein_profile_occupancy(void)
{
    struct imm_amino const *amino = &imm_amino_iupac;
    struct imm_nuclt const *nuclt = imm_super(&imm_dna_iupac);
    struct imm_nuclt_code code;
    imm_nuclt_code_init(&code, nuclt);
    struct prot_cfg cfg = prot_cfg(ENTRY_DIST_OCCUPANCY, 0.1f);

    struct prot_profile prof;
    prot_profile_init(&prof, "accession", amino, &code, cfg);
    eq(prot_profile_sample(&prof, 1, 2), RC_OK);

    char const str[] = "ATGAAACGCATTAGCACCACCATTACCACCAC";
    struct imm_seq seq = imm_seq(imm_str(str), prof.super.code->abc);

    eq(prot_profile_setup(&prof, imm_seq_size(&seq), true, false), RC_OK);

    struct imm_prod prod = imm_prod();
    struct imm_dp *dp = &prof.null.dp;
    struct imm_task *task = imm_task_new(dp);
    notnull(task);
    eq(imm_task_setup(task, &seq), IMM_OK);
    eq(imm_dp_viterbi(dp, task, &prod), IMM_OK);

    close(prod.loglik, -48.9272687711);

    eq(imm_path_nsteps(&prod.path), 11);
    char name[IMM_STATE_NAME_SIZE];

    eq(imm_path_step(&prod.path, 0)->seqlen, 3);
    eq(imm_path_step(&prod.path, 0)->state_id, PROTEIN_R_STATE);
    prot_state_name(imm_path_step(&prod.path, 0)->state_id, name);
    eq(name, "R");

    eq(imm_path_step(&prod.path, 10)->seqlen, 2);
    eq(imm_path_step(&prod.path, 10)->state_id, PROTEIN_R_STATE);
    prot_state_name(imm_path_step(&prod.path, 10)->state_id, name);
    eq(name, "R");

    imm_prod_reset(&prod);
    imm_del(task);

    dp = &prof.alt.dp;
    task = imm_task_new(dp);
    notnull(task);
    eq(imm_task_setup(task, &seq), IMM_OK);
    eq(imm_dp_viterbi(dp, task, &prod), IMM_OK);

    close(prod.loglik, -54.35543421312);

    eq(imm_path_nsteps(&prod.path), 14);

    eq(imm_path_step(&prod.path, 0)->seqlen, 0);
    eq(imm_path_step(&prod.path, 0)->state_id, PROTEIN_S_STATE);
    prot_state_name(imm_path_step(&prod.path, 0)->state_id, name);
    eq(name, "S");

    eq(imm_path_step(&prod.path, 13)->seqlen, 0);
    eq(imm_path_step(&prod.path, 13)->state_id, PROTEIN_T_STATE);
    prot_state_name(imm_path_step(&prod.path, 13)->state_id, name);
    eq(name, "T");

    struct prot_codec codec = prot_codec_init(&prof, &prod.path);
    enum rc rc = RC_OK;

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
    while (!(rc = prot_codec_next(&codec, &seq, &codon)))
    {
        eq(codons[i].a, codon.a);
        eq(codons[i].b, codon.b);
        eq(codons[i].c, codon.c);
        ++i;
    }
    eq(rc, RC_END);
    eq(i, 10);

    profile_del((struct profile *)&prof);
    imm_del(&prod);
    imm_del(task);
}
