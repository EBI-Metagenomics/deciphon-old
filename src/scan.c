#include "scan.h"
#include "model.h"
#include "nmm/nmm.h"
#include "profile.h"
#include "result.h"
#include "seq.h"

static void model_scan(struct imm_hmm* hmm, struct imm_dp* dp, struct imm_seq const* seq, bool calc_loglik,
                       struct model* model);

int scan(struct dcp_profile const* profile, struct seq const* seq, struct dcp_result* result,
         struct dcp_task_cfg const* cfg)
{
    struct imm_abc const* abc = dcp_profile_abc(profile);
    struct imm_seq const* iseq = imm_seq_create(seq_string(seq), abc);
    if (!iseq)
        return 1;

    struct imm_hmm* hmm = imm_model_hmm(dcp_profile_model(profile, 0));
    struct imm_dp*  dp = imm_model_dp(dcp_profile_model(profile, 0));
    profile_setup(hmm, dp, cfg->multiple_hits, imm_seq_length(iseq), cfg->hmmer3_compat);
    model_scan(hmm, dp, iseq, cfg->loglik, result_model(result, DCP_MODEL_ALT));

    struct model* null = result_model(result, DCP_MODEL_NULL);
    model_set_loglik(null, imm_lprob_invalid());
    model_set_result(null, null->result);
    if (cfg->null) {
        hmm = imm_model_hmm(dcp_profile_model(profile, 1));
        dp = imm_model_dp(dcp_profile_model(profile, 1));
        model_scan(hmm, dp, iseq, cfg->loglik, null);
    }

    imm_seq_destroy(iseq);
    return 0;
}

static void model_scan(struct imm_hmm* hmm, struct imm_dp* dp, struct imm_seq const* seq, bool calc_loglik,
                       struct model* model)
{
    struct imm_dp_task* task = imm_dp_task_create(dp);

    imm_dp_task_setup(task, seq);
    struct imm_result const* r = imm_dp_viterbi(dp, task);
    imm_float                loglik = imm_lprob_invalid();
    if (calc_loglik && !imm_path_empty(imm_result_path(r)))
        loglik = imm_hmm_loglikelihood(hmm, seq, imm_result_path(r));

    model_set_loglik(model, loglik);
    model_set_result(model, r);

    imm_dp_task_destroy(task);

    struct imm_path const* path = imm_result_path(r);
    struct imm_step const* step = imm_path_first(path);

    step = imm_path_first(path);
    string_reuse(&model->path);
    while (step) {
        char const* name = imm_state_get_name(imm_step_state(step));

        while (*name != '\0')
            string_set(&model->path, *(name++));

        string_set(&model->path, ':');

        if (imm_step_seq_len(step) == 0)
            string_set(&model->path, '0');
        else if (imm_step_seq_len(step) == 1)
            string_set(&model->path, '1');
        else if (imm_step_seq_len(step) == 2)
            string_set(&model->path, '2');
        else if (imm_step_seq_len(step) == 3)
            string_set(&model->path, '3');
        else if (imm_step_seq_len(step) == 4)
            string_set(&model->path, '4');
        else if (imm_step_seq_len(step) == 5)
            string_set(&model->path, '5');
        else if (imm_step_seq_len(step) == 6)
            string_set(&model->path, '6');
        else if (imm_step_seq_len(step) == 7)
            string_set(&model->path, '7');
        else if (imm_step_seq_len(step) == 8)
            string_set(&model->path, '8');
        else if (imm_step_seq_len(step) == 9)
            string_set(&model->path, '9');

        string_set(&model->path, ',');

        step = imm_path_next(path, step);
    }
    string_rollback(&model->path);
    string_setend(&model->path);

    path = imm_result_path(r);
    step = imm_path_first(path);
    NMM_CODON_DECL(codon, nmm_base_abc_derived(imm_seq_get_abc(seq)));
    uint32_t offset = 0;
    string_reuse(&model->codons);
    while (step) {
        struct imm_state const* state = imm_step_state(step);
        if (imm_state_type_id(state) == NMM_FRAME_STATE_TYPE_ID) {

            struct nmm_frame_state const* f = nmm_frame_state_derived(state);
            struct imm_seq                subseq = IMM_SUBSEQ(seq, offset, imm_step_seq_len(step));
            nmm_frame_state_decode(f, &subseq, &codon);
            string_set(&model->codons, codon.a);
            string_set(&model->codons, codon.b);
            string_set(&model->codons, codon.c);
        }
        offset += imm_step_seq_len(step);
        step = imm_path_next(path, step);
    }
    string_setend(&model->codons);
}
