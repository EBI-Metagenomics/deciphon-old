#include "misc.h"
#include "bug.h"
#include <string.h>

struct special_trans target_length_model(bool multiple_hits, uint32_t target_length, bool hmmer3_compat)
{
    imm_float L = (imm_float)target_length;
    BUG(L == 0);

    imm_float q = 0.0;
    imm_float log_q = imm_lprob_zero();

    if (multiple_hits) {
        q = 0.5;
        log_q = imm_log(0.5);
    }

    imm_float lp = imm_log(L) - imm_log(L + 2 + q / (1 - q));
    imm_float l1p = imm_log(2 + q / (1 - q)) - imm_log(L + 2 + q / (1 - q));
    imm_float lr = imm_log(L) - imm_log(L + 1);

    struct special_trans t;

    t.NN = t.CC = t.JJ = lp;
    t.NB = t.CT = t.JB = l1p;
    t.RR = lr;
    t.EJ = log_q;
    t.EC = imm_log(1 - q);

    if (hmmer3_compat) {
        t.NN = 0.;
        t.CC = 0.;
        t.JJ = 0.;
    }

    return t;
}

void set_special_trans(struct special_trans trans, struct imm_hmm* hmm, struct imm_dp* dp)
{
    struct imm_state const* S = NULL;
    struct imm_state const* B = NULL;
    struct imm_state const* N = NULL;
    struct imm_state const* E = NULL;
    struct imm_state const* T = NULL;
    struct imm_state const* C = NULL;
    struct imm_state const* J = NULL;
    struct imm_state const* D2 = NULL;
    struct imm_state const* I1 = NULL;

    uint32_t                 nstates = 0;
    struct imm_state const** states = imm_hmm_states(hmm, &nstates);
    for (uint32_t i = 0; i < nstates; ++i) {

        char const* name = imm_state_get_name(states[i]);
        if (strcmp(name, "S") == 0)
            S = states[i];
        else if (strcmp(name, "B") == 0)
            B = states[i];
        else if (strcmp(name, "N") == 0)
            N = states[i];
        else if (strcmp(name, "E") == 0)
            E = states[i];
        else if (strcmp(name, "T") == 0)
            T = states[i];
        else if (strcmp(name, "C") == 0)
            C = states[i];
        else if (strcmp(name, "J") == 0)
            J = states[i];
        else if (strcmp(name, "D2") == 0)
            D2 = states[i];
        else if (strcmp(name, "I1") == 0)
            I1 = states[i];
    }
    imm_dp_change_trans(dp, hmm, S, B, trans.NB);
    imm_dp_change_trans(dp, hmm, S, N, trans.NN);
    imm_dp_change_trans(dp, hmm, N, N, trans.NN);
    imm_dp_change_trans(dp, hmm, N, B, trans.NB);

    imm_dp_change_trans(dp, hmm, E, T, trans.EC + trans.CT);
    imm_dp_change_trans(dp, hmm, E, C, trans.EC + trans.CC);
    imm_dp_change_trans(dp, hmm, C, C, trans.CC);
    imm_dp_change_trans(dp, hmm, C, T, trans.CT);

    imm_dp_change_trans(dp, hmm, E, B, trans.EJ + trans.JB);
    imm_dp_change_trans(dp, hmm, E, J, trans.EJ + trans.JJ);
    imm_dp_change_trans(dp, hmm, J, J, trans.JJ);
    imm_dp_change_trans(dp, hmm, J, B, trans.JB);

    imm_dp_change_trans(dp, hmm, B, D2, imm_lprob_zero());
    imm_dp_change_trans(dp, hmm, B, I1, imm_lprob_zero());
}
