#include "dcp/dcp.h"
#include "hope/hope.h"

void test_pro_prof_uniform(void);
void test_pro_prof_occupancy(void);

int main(void)
{
    test_pro_prof_uniform();
    test_pro_prof_occupancy();
    return hope_status();
}

void test_pro_prof_uniform(void)
{
    struct imm_amino const *amino = &imm_amino_iupac;
    struct imm_nuclt const *nuclt = imm_super(&imm_dna_iupac);
    struct dcp_pro_cfg cfg = dcp_pro_cfg(DCP_ENTRY_DIST_UNIFORM, 0.1f);

    struct dcp_pro_prof prof;
    dcp_pro_prof_init(&prof, amino, nuclt, cfg);
    EQ(dcp_pro_prof_sample(&prof, 1, 2), DCP_SUCCESS);

    char const str[] = "ATGAAACGCATTAGCACCACCATTACCACCAC";
    struct imm_seq seq = imm_seq(imm_str(str), dcp_super(&prof)->abc);

    EQ(dcp_pro_prof_setup(&prof, 0, true, false), DCP_ILLEGALARG);
    EQ(dcp_pro_prof_setup(&prof, imm_seq_size(&seq), true, false), DCP_SUCCESS);

    struct imm_result result = imm_result();
    struct imm_dp *dp = &prof.null.dp;
    struct imm_task *task = imm_task_new(dp);
    NOTNULL(task);
    EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
    EQ(imm_dp_viterbi(dp, task, &result), IMM_SUCCESS);

    CLOSE(result.loglik, -48.9272687711);

    EQ(imm_path_nsteps(&result.path), 11);
    char name[IMM_STATE_NAME_SIZE];

    EQ(imm_path_step(&result.path, 0)->seqlen, 3);
    EQ(imm_path_step(&result.path, 0)->state_id, DCP_PRO_ID_R);
    dcp_pro_prof_state_name(imm_path_step(&result.path, 0)->state_id, name);
    EQ(name, "R");

    EQ(imm_path_step(&result.path, 10)->seqlen, 2);
    EQ(imm_path_step(&result.path, 10)->state_id, DCP_PRO_ID_R);
    dcp_pro_prof_state_name(imm_path_step(&result.path, 10)->state_id, name);
    EQ(name, "R");

    imm_result_reset(&result);
    imm_del(task);

    dp = &prof.alt.dp;
    task = imm_task_new(dp);
    NOTNULL(task);
    EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
    EQ(imm_dp_viterbi(dp, task, &result), IMM_SUCCESS);

    CLOSE(result.loglik, -55.59428153448);

    EQ(imm_path_nsteps(&result.path), 14);

    EQ(imm_path_step(&result.path, 0)->seqlen, 0);
    EQ(imm_path_step(&result.path, 0)->state_id, DCP_PRO_ID_S);
    dcp_pro_prof_state_name(imm_path_step(&result.path, 0)->state_id, name);
    EQ(name, "S");

    EQ(imm_path_step(&result.path, 13)->seqlen, 0);
    EQ(imm_path_step(&result.path, 13)->state_id, DCP_PRO_ID_T);
    dcp_pro_prof_state_name(imm_path_step(&result.path, 13)->state_id, name);
    EQ(name, "T");

    struct dcp_pro_codec codec = dcp_pro_codec_init(&prof, &result.path);
    enum dcp_rc rc = DCP_SUCCESS;

    struct imm_codon codons[10] = {__imm_codon_from_str(prof.nuclt, "ATG"),
                                   __imm_codon_from_str(prof.nuclt, "AAA"),
                                   __imm_codon_from_str(prof.nuclt, "CGC"),
                                   __imm_codon_from_str(prof.nuclt, "ATA"),
                                   __imm_codon_from_str(prof.nuclt, "GCA"),
                                   __imm_codon_from_str(prof.nuclt, "TGA"),
                                   __imm_codon_from_str(prof.nuclt, "CAT"),
                                   __imm_codon_from_str(prof.nuclt, "TGA"),
                                   __imm_codon_from_str(prof.nuclt, "TGA"),
                                   __imm_codon_from_str(prof.nuclt, "TGA")};

    char any = imm_abc_any_symbol(imm_super(prof.nuclt));
    struct imm_codon codon = imm_codon(prof.nuclt, any, any, any);
    unsigned i = 0;
    while (!(rc = dcp_pro_codec_next(&codec, &seq, &codon)))
    {
        printf("%d: codon(%c%c%c)\n", i, imm_codon_asym(&codon),
               imm_codon_bsym(&codon), imm_codon_csym(&codon));
        fflush(stdout);
        EQ(codons[i].a, codon.a);
        EQ(codons[i].b, codon.b);
        EQ(codons[i].c, codon.c);
        ++i;
    }
    EQ(rc, DCP_END);
    EQ(i, 10);

    dcp_del(&prof);
    imm_del(&result);
    imm_del(task);
}

void test_pro_prof_occupancy(void)
{
    struct imm_amino const *amino = &imm_amino_iupac;
    struct imm_nuclt const *nuclt = imm_super(&imm_dna_iupac);
    struct dcp_pro_cfg cfg = dcp_pro_cfg(DCP_ENTRY_DIST_OCCUPANCY, 0.1f);

    struct dcp_pro_prof prof;
    dcp_pro_prof_init(&prof, amino, nuclt, cfg);
    EQ(dcp_pro_prof_sample(&prof, 1, 2), DCP_SUCCESS);

    char const str[] = "ATGAAACGCATTAGCACCACCATTACCACCAC";
    struct imm_seq seq = imm_seq(imm_str(str), dcp_super(&prof)->abc);

    EQ(dcp_pro_prof_setup(&prof, imm_seq_size(&seq), true, false), DCP_SUCCESS);

    struct imm_result result = imm_result();
    struct imm_dp *dp = &prof.null.dp;
    struct imm_task *task = imm_task_new(dp);
    NOTNULL(task);
    EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
    EQ(imm_dp_viterbi(dp, task, &result), IMM_SUCCESS);

    CLOSE(result.loglik, -48.9272687711);

    EQ(imm_path_nsteps(&result.path), 11);
    char name[IMM_STATE_NAME_SIZE];

    EQ(imm_path_step(&result.path, 0)->seqlen, 3);
    EQ(imm_path_step(&result.path, 0)->state_id, DCP_PRO_ID_R);
    dcp_pro_prof_state_name(imm_path_step(&result.path, 0)->state_id, name);
    EQ(name, "R");

    EQ(imm_path_step(&result.path, 10)->seqlen, 2);
    EQ(imm_path_step(&result.path, 10)->state_id, DCP_PRO_ID_R);
    dcp_pro_prof_state_name(imm_path_step(&result.path, 10)->state_id, name);
    EQ(name, "R");

    imm_result_reset(&result);
    imm_del(task);

    dp = &prof.alt.dp;
    task = imm_task_new(dp);
    NOTNULL(task);
    EQ(imm_task_setup(task, &seq), IMM_SUCCESS);
    EQ(imm_dp_viterbi(dp, task, &result), IMM_SUCCESS);

    CLOSE(result.loglik, -54.35543421312);

    EQ(imm_path_nsteps(&result.path), 14);

    EQ(imm_path_step(&result.path, 0)->seqlen, 0);
    EQ(imm_path_step(&result.path, 0)->state_id, DCP_PRO_ID_S);
    dcp_pro_prof_state_name(imm_path_step(&result.path, 0)->state_id, name);
    EQ(name, "S");

    EQ(imm_path_step(&result.path, 13)->seqlen, 0);
    EQ(imm_path_step(&result.path, 13)->state_id, DCP_PRO_ID_T);
    dcp_pro_prof_state_name(imm_path_step(&result.path, 13)->state_id, name);
    EQ(name, "T");

    dcp_del(&prof);
    imm_del(&result);
    imm_del(task);
}
