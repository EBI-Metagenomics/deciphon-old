#include "dcp/dcp.h"
#include "hope/hope.h"
#include "imm/imm.h"

#define FILEPATH "/Users/horta/data/PF02545_cut.hmm"
#define OUTPUT "/Users/horta/tmp/PF02545_cut_dcp.dot"

#define FILEPATH2 "/Users/horta/data/PF02545.hmm"
#define OUTPUT2 "/Users/horta/tmp/PF02545_cut_dcp2.dot"

void test_pro_reader(void);
void test_pro_reader2(void);

int main(void)
{
    /* test_pro_reader(); */
    test_pro_reader2();
    return hope_status();
}

void test_pro_reader(void)
{
    struct dcp_pro_reader reader;
    FILE *fd = fopen(FILEPATH, "r");
    NOTNULL(fd);
    struct dcp_pro_cfg cfg = {&imm_amino_iupac, imm_super(imm_gc_dna()),
                              DCP_ENTRY_DIST_OCCUPANCY, 0.01f};
    dcp_pro_reader_init(&reader, cfg, fd);

    enum dcp_rc rc = dcp_pro_reader_next(&reader);
    EQ(rc, DCP_SUCCESS);

    FILE *dot_fd = fopen(OUTPUT, "w");
    dcp_pro_model_write_dot(&reader.model, dot_fd);
    fclose(dot_fd);

    struct dcp_pro_profile p;
    dcp_pro_profile_init(&p, cfg);

    dcp_profile_nameit(dcp_super(&p), dcp_meta("name", "acc"));
    EQ(dcp_pro_profile_absorb(&p, &reader.model), DCP_SUCCESS);

    /* char const str[] = "CCTGGTAAAGAAGATAATAACAAA"; */
    char const str[] = "CCTGGTAAAGAAGATAATAACAAA";
    struct imm_seq seq = imm_seq(imm_str(str), dcp_super(&p)->abc);

    dcp_pro_profile_setup(&p, imm_seq_size(&seq), true, false);

    struct imm_result result = imm_result();
    struct imm_dp *dp = &p.alt.dp;
    struct imm_task *task = imm_task_new(dp);
    imm_task_setup(task, &seq);
    imm_dp_viterbi(dp, task, &result);

    CLOSE(result.loglik, -37.60375230420982);

    dcp_del(&p);

    fclose(fd);
}

void test_pro_reader2(void)
{
    struct dcp_pro_reader reader;
    FILE *fd = fopen(FILEPATH2, "r");
    NOTNULL(fd);
    struct dcp_pro_cfg cfg = {&imm_amino_iupac, imm_super(imm_gc_dna()),
                              DCP_ENTRY_DIST_OCCUPANCY, 0.01f};
    dcp_pro_reader_init(&reader, cfg, fd);

    enum dcp_rc rc = dcp_pro_reader_next(&reader);
    EQ(rc, DCP_SUCCESS);

    FILE *dot_fd = fopen(OUTPUT2, "w");
    dcp_pro_model_write_dot(&reader.model, dot_fd);
    fclose(dot_fd);

    struct dcp_pro_profile p;
    dcp_pro_profile_init(&p, cfg);

    dcp_profile_nameit(dcp_super(&p), dcp_meta("name", "acc"));
    EQ(dcp_pro_profile_absorb(&p, &reader.model), DCP_SUCCESS);

    char const str[] = "CCTGGTAAA";
    struct imm_seq seq = imm_seq(imm_str(str), dcp_super(&p)->abc);

    dcp_pro_profile_setup(&p, imm_seq_size(&seq), true, false);

    struct imm_result result = imm_result();
    struct imm_dp *dp = &p.alt.dp;
    struct imm_task *task = imm_task_new(dp);
    imm_task_setup(task, &seq);
    imm_dp_viterbi(dp, task, &result);

    CLOSE(result.loglik, -21.98851860153052);
    /* ~/tmp/e3.fasta */
    /* ~/data/PF02545.hmm */
    /* <Path:<S,0>,<N,3>,<B,0>,<M61,3>,<M62,3>,<E,0>,<T,0>> */
    for (unsigned i = 0; i < imm_path_nsteps(&result.path); ++i)
    {
        struct imm_step *step = imm_path_step(&result.path, i);
        char name[IMM_STATE_NAME_SIZE];
        dcp_pro_profile_state_name(step->state_id, name);
        printf("<%s,%d>,", name, step->seqlen);
    }
    printf("\n");

    char const tstr0[] = "CCT";
    struct imm_seq tseq0 = imm_seq(imm_str(tstr0), dcp_super(&p)->abc);
    char const tstr1[] = "GGT";
    struct imm_seq tseq1 = imm_seq(imm_str(tstr1), dcp_super(&p)->abc);
    char const tstr2[] = "AAA";
    struct imm_seq tseq2 = imm_seq(imm_str(tstr2), dcp_super(&p)->abc);

    printf("<N,%s>: %f\n", tstr0,
           imm_state_lprob(imm_super(&reader.model.ext_node.alt.N), &tseq0));

    printf("<C,%s>: %f\n", tstr1,
           imm_state_lprob(imm_super(&reader.model.ext_node.alt.C), &tseq1));

    printf("<C,%s>: %f\n", tstr2,
           imm_state_lprob(imm_super(&reader.model.ext_node.alt.C), &tseq2));

    printf("<N,%s>: %f\n", tstr0,
           imm_dp_emis_score(dp, reader.model.ext_node.alt.N.super.id, &tseq0));

    printf("<C,%s>: %f\n", tstr1,
           imm_state_lprob(imm_super(&reader.model.ext_node.alt.C), &tseq1));

    printf("<C,%s>: %f\n", tstr2,
           imm_state_lprob(imm_super(&reader.model.ext_node.alt.C), &tseq2));

    imm_dp_dump_path(dp, task, &result);

    /* (Pdb) !self._alt_model._special_node.N.lprob(imm.Sequence.create(b"CCT",
     * self._alphabet)) */
    /* -4.456590403799719 */
    /* (Pdb) !self._alt_model._special_node.C.lprob(imm.Sequence.create(b"GGT",
     * self._alphabet)) */
    /* -4.0924379971948515 */
    /* (Pdb) !self._alt_model._special_node.C.lprob(imm.Sequence.create(b"AAA",
     * self._alphabet)) */
    /* -3.5557730364450753 */

    /* char name[IMM_STATE_NAME_SIZE]; */
    /* dcp_pro_profile_state_name(reader.model.alt.nodes[60].M.super.id, name);
     */
    /* printf("<%s,%s>: %f\n", name, tstr1, */
    /*        imm_state_lprob(imm_super(&reader.model.alt.nodes[60].M),
     * &tseq1)); */

    /* char name[IMM_STATE_NAME_SIZE]; */
    /* dcp_pro_profile_state_name(reader.model.alt.nodes[61].M.super.id, name);
     */
    /* printf("<%s,%s>: %f\n", name, tstr2, */
    /*        imm_state_lprob(imm_super(&reader.model.alt.nodes[61].M),
     * &tseq2)); */

    /* char name[IMM_STATE_NAME_SIZE]; */
    /* dcp_pro_profile_state_name(reader.model.alt.nodes[62].M.super.id, name);
     */
    /* printf("<%s,%s>: %f\n", name, tstr0, */
    /*        imm_state_lprob(imm_super(&reader.model.alt.nodes[62].M),
     * &tseq0)); */

    /* char const tstr[] = "CCT"; */
    /* struct imm_seq tseq = imm_seq(imm_str(tstr), dcp_super(&p)->abc); */
    /* struct imm_path tpath = imm_path(); */
    /* imm_path_add(&tpath, imm_step(DCP_PRO_ID_S, 0)); */
    /* imm_path_add(&tpath, imm_step(DCP_PRO_ID_N, 3)); */
    /* imm_path_add(&tpath, imm_step(DCP_PRO_ID_B, 0)); */
    /* imm_path_add(&tpath, imm_step(DCP_PRO_ID_MATCH | 61, 3)); */
    /* imm_path_add(&tpath, imm_step(DCP_PRO_ID_MATCH | 62, 3)); */
    /* imm_path_add(&tpath, imm_step(DCP_PRO_ID_E, 0)); */
    /* imm_path_add(&tpath, imm_step(DCP_PRO_ID_T, 0)); */
    /* printf("%f\n", imm_hmm_loglik(&reader.model.alt.hmm, &seq, &tpath)); */

    dcp_del(&p);

    fclose(fd);
}
