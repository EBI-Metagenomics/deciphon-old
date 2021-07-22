#include "dcp/dcp.h"
#include "hope/hope.h"
#include "imm/imm.h"

void test_normal_profile(void);

int main(void)
{
    test_normal_profile();
    return hope_status();
}

void test_normal_profile(void)
{
    imm_example1_init();
    struct imm_example1 *m = &imm_example1;

    /* Profile 0 */
    struct dcp_normal_profile *prof0 =
        dcp_normal_profile_new(&m->abc, dcp_meta("NAME0", "ACC0"));
    imm_hmm_reset_dp(m->null.hmm, imm_super(m->null.n), prof0->dp.null);
    imm_hmm_reset_dp(m->hmm, imm_super(m->end), prof0->dp.alt);

    /* Profile 1 */
    struct imm_mute_state *state = imm_mute_state_new(3, &m->abc);
    struct imm_hmm *hmm = imm_hmm_new(&m->abc);
    EQ(imm_hmm_add_state(hmm, imm_super(state)), IMM_SUCCESS);
    EQ(imm_hmm_set_start(hmm, imm_super(state), imm_log(0.3)), IMM_SUCCESS);
    struct dcp_normal_profile *prof1 =
        dcp_normal_profile_new(&m->abc, dcp_meta("NAME1", "ACC1"));
    imm_hmm_reset_dp(m->null.hmm, imm_super(state), prof1->dp.null);
    imm_hmm_reset_dp(m->hmm, imm_super(state), prof1->dp.alt);
    imm_del(hmm);
    imm_del(state);

    dcp_normal_profile_del(prof0);
    dcp_normal_profile_del(prof1);
    imm_example1_deinit();
}
