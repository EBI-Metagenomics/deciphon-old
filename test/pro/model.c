#include "dcp/dcp.h"
#include "hmr/hmr.h"
#include "hope/hope.h"
#include "imm/imm.h"

void test_pro_model(void);
void test_pro_model_from_hmmfile(void);

int main(void)
{
    test_pro_model();
    /* test_pro_model_from_hmmfile(); */
    return hope_status();
}

void test_pro_model(void)
{
    unsigned core_size = 3;
    struct dcp_pro_cfg cfg = {&imm_amino_iupac, imm_super(imm_gc_dna()),
                              DCP_ENTRY_DIST_OCCUPANCY, 0.01f};
    imm_float null_lprobs[IMM_AMINO_SIZE];
    imm_float null_lodds[IMM_AMINO_SIZE];
    imm_float match_lprobs1[IMM_AMINO_SIZE];
    imm_float match_lprobs2[IMM_AMINO_SIZE];
    imm_float match_lprobs3[IMM_AMINO_SIZE];
    struct dcp_pro_trans t[4];

    struct imm_rnd rnd = imm_rnd(942);
    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, null_lprobs);
    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, null_lodds);
    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, match_lprobs1);
    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, match_lprobs2);
    imm_lprob_sample(&rnd, IMM_AMINO_SIZE, match_lprobs3);

    for (unsigned i = 0; i < 4; ++i)
    {
        imm_lprob_sample(&rnd, DCP_PRO_TRANS_SIZE, t[i].data);
        imm_lprob_normalize(DCP_PRO_TRANS_SIZE, t[i].data);
    }

    struct dcp_pro_model model;
    enum dcp_rc rc = dcp_pro_model_init(&model, cfg, null_lprobs, null_lodds);
    EQ(rc, DCP_SUCCESS);

    EQ(dcp_pro_model_setup(&model, core_size), DCP_SUCCESS);

    EQ(dcp_pro_model_add_node(&model, match_lprobs1), DCP_SUCCESS);
    EQ(dcp_pro_model_add_node(&model, match_lprobs2), DCP_SUCCESS);
    EQ(dcp_pro_model_add_node(&model, match_lprobs3), DCP_SUCCESS);

    EQ(dcp_pro_model_add_trans(&model, t[0]), DCP_SUCCESS);
    EQ(dcp_pro_model_add_trans(&model, t[1]), DCP_SUCCESS);
    EQ(dcp_pro_model_add_trans(&model, t[2]), DCP_SUCCESS);
    EQ(dcp_pro_model_add_trans(&model, t[3]), DCP_SUCCESS);

    struct dcp_pro_prof p;
    dcp_pro_prof_init(&p, cfg);

    dcp_prof_nameit(dcp_super(&p), dcp_meta("NAME0", "ACC0"));
    EQ(dcp_pro_prof_absorb(&p, &model), DCP_SUCCESS);

    dcp_del(&p);
    dcp_del(&model);
}

void test_pro_model_from_hmmfile(void)
{
    FILE *fd = fopen("//Users/horta/data/PF02545.hmm", "r");
    NOTNULL(fd);

    HMR_DECLARE(hmr, fd);

    HMR_PROF_DECLARE(prof, &hmr);

    struct dcp_pro_cfg cfg = {&imm_amino_iupac, imm_super(imm_gc_dna()),
                              DCP_ENTRY_DIST_OCCUPANCY, 0.01f};

    /* Copy/paste from HMMER3 amino acid frequences inferred from Swiss-Prot
     * 50.8, (Oct 2006), counting over 85956127 (86.0M) residues. */
    imm_float null_lprobs[IMM_AMINO_SIZE] = {
        imm_log(0.0787945), /*"A":*/
        imm_log(0.0151600), /*"C":*/
        imm_log(0.0535222), /*"D":*/
        imm_log(0.0668298), /*"E":*/
        imm_log(0.0397062), /*"F":*/
        imm_log(0.0695071), /*"G":*/
        imm_log(0.0229198), /*"H":*/
        imm_log(0.0590092), /*"I":*/
        imm_log(0.0594422), /*"K":*/
        imm_log(0.0963728), /*"L":*/
        imm_log(0.0237718), /*"M":*/
        imm_log(0.0414386), /*"N":*/
        imm_log(0.0482904), /*"P":*/
        imm_log(0.0395639), /*"Q":*/
        imm_log(0.0540978), /*"R":*/
        imm_log(0.0683364), /*"S":*/
        imm_log(0.0540687), /*"T":*/
        imm_log(0.0673417), /*"V":*/
        imm_log(0.0114135), /*"W":*/
        imm_log(0.0304133), /*"Y":*/
    };

    imm_float null_lodds[IMM_AMINO_SIZE];
    for (unsigned i = 0; i < IMM_AMINO_SIZE; ++i)
        null_lodds[i] = 0.0f;

    unsigned prof_idx = 0;
    enum hmr_rc rc = HMR_SUCCESS;
    while (!(rc = hmr_next_prof(&hmr, &prof)))
    {
        struct dcp_pro_model model;
        EQ(dcp_pro_model_init(&model, cfg, null_lprobs, null_lodds),
           DCP_SUCCESS);

        unsigned core_size = hmr_prof_length(&prof);
        EQ(dcp_pro_model_setup(&model, core_size), DCP_SUCCESS);

        rc = hmr_next_node(&hmr, &prof);
        EQ(rc, HMR_SUCCESS);

        struct dcp_pro_trans t = {
            .MM = (imm_float)prof.node.trans[HMR_TRANS_MM],
            .MI = (imm_float)prof.node.trans[HMR_TRANS_MI],
            .MD = (imm_float)prof.node.trans[HMR_TRANS_MD],
            .IM = (imm_float)prof.node.trans[HMR_TRANS_IM],
            .II = (imm_float)prof.node.trans[HMR_TRANS_II],
            .DM = (imm_float)prof.node.trans[HMR_TRANS_DM],
            .DD = (imm_float)prof.node.trans[HMR_TRANS_DD],
        };
        EQ(dcp_pro_model_add_trans(&model, t), DCP_SUCCESS);
        /* prof->node.trans[] */
        /* double trans[HMR_TRANS_SIZE]; */

        unsigned node_idx = 1;
        while (!(rc = hmr_next_node(&hmr, &prof)))
        {
            imm_float match_lprobs[IMM_AMINO_SIZE];
            for (unsigned i = 0; i < IMM_AMINO_SIZE; ++i)
                match_lprobs[i] = (imm_float)prof.node.match[i];

            EQ(dcp_pro_model_add_node(&model, match_lprobs), DCP_SUCCESS);

            struct dcp_pro_trans t2 = {
                .MM = (imm_float)prof.node.trans[HMR_TRANS_MM],
                .MI = (imm_float)prof.node.trans[HMR_TRANS_MI],
                .MD = (imm_float)prof.node.trans[HMR_TRANS_MD],
                .IM = (imm_float)prof.node.trans[HMR_TRANS_IM],
                .II = (imm_float)prof.node.trans[HMR_TRANS_II],
                .DM = (imm_float)prof.node.trans[HMR_TRANS_DM],
                .DD = (imm_float)prof.node.trans[HMR_TRANS_DD],
            };
            EQ(dcp_pro_model_add_trans(&model, t2), DCP_SUCCESS);

            node_idx++;
        }
        /* dcp_pro_model_write_dot(model, stdout); */

        struct dcp_pro_prof p;
        dcp_pro_prof_init(&p, cfg);

        dcp_prof_nameit(dcp_super(&p), dcp_meta(prof.meta.name, prof.meta.acc));
        EQ(dcp_pro_prof_absorb(&p, &model), DCP_SUCCESS);

        char const str[] = "CCTGGTAAAGAAGATAATAACAAA";
        struct imm_seq seq = imm_seq(imm_str(str), dcp_super(&p)->abc);

        dcp_pro_prof_setup(&p, imm_seq_size(&seq), true, false);

        struct imm_result result = imm_result();
        struct imm_dp *dp = &p.alt.dp;
        struct imm_task *task = imm_task_new(dp);
        imm_task_setup(task, &seq);
        imm_dp_viterbi(dp, task, &result);

        CLOSE(result.loglik, -43.40209579468);

        dcp_del(&p);
        dcp_del(&model);

        prof_idx++;
    }

    fclose(fd);

#if 0
    for (unsigned i = 0; i < 4; ++i)
    {
        imm_lprob_sample(&rnd, DCP_PRO_MODEL_TRANS_SIZE, t[i].data);
        imm_lprob_normalize(DCP_PRO_MODEL_TRANS_SIZE, t[i].data);
    }

    struct dcp_pro_model *model =
        dcp_pro_model_new(cfg, null_lprobs, null_lodds);

    EQ(dcp_pro_model_setup(model, core_size), DCP_SUCCESS);

    EQ(dcp_pro_model_add_node(model, match_lprobs1), DCP_SUCCESS);
    EQ(dcp_pro_model_add_node(model, match_lprobs2), DCP_SUCCESS);
    EQ(dcp_pro_model_add_node(model, match_lprobs3), DCP_SUCCESS);

    EQ(dcp_pro_model_add_trans(model, t[0]), DCP_SUCCESS);
    EQ(dcp_pro_model_add_trans(model, t[1]), DCP_SUCCESS);
    EQ(dcp_pro_model_add_trans(model, t[2]), DCP_SUCCESS);
    EQ(dcp_pro_model_add_trans(model, t[3]), DCP_SUCCESS);

    struct dcp_pro_profile p;
    dcp_pro_profile_init(&p, cfg);

    dcp_prof_nameit(dcp_super(&p), dcp_meta("NAME0", "ACC0"));
    EQ(dcp_pro_profile_absorb(&p, model), DCP_SUCCESS);

    dcp_del(&p);
    dcp_del(model);
#endif
}
