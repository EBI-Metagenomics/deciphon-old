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
    struct imm_dp *null0 = imm_hmm_new_dp(m->null.hmm, imm_super(m->null.n));
    struct imm_dp *alt0 = imm_hmm_new_dp(m->hmm, imm_super(m->end));
    struct dcp_normal_profile *prof0 = dcp_normal_profile_new(
        &m->abc, dcp_metadata("NAME0", "ACC0"), null0, alt0);

    /* Profile 1 */
    struct imm_mute_state *state = imm_mute_state_new(3, &m->abc);
    struct imm_hmm *hmm = imm_hmm_new(&m->abc);
    EQ(imm_hmm_add_state(hmm, imm_super(state)), IMM_SUCCESS);
    EQ(imm_hmm_set_start(hmm, imm_super(state), imm_log(0.3)), IMM_SUCCESS);
    struct imm_dp *null1 = imm_hmm_new_dp(m->null.hmm, imm_super(state));
    struct imm_dp *alt1 = imm_hmm_new_dp(m->hmm, imm_super(state));
    struct dcp_normal_profile *prof1 = dcp_normal_profile_new(
        &m->abc, dcp_metadata("NAME1", "ACC1"), null1, alt1);
    /* prof.idx = 1; */
    /* prof.mt = dcp_metadata("NAME1", "ACC1"); */
    /* EQ(imm_hmm_reset_dp(hmm, imm_super(state), prof.dp.null), IMM_SUCCESS);
     */
    /* EQ(imm_hmm_reset_dp(hmm, imm_super(state), prof.dp.alt), IMM_SUCCESS); */
    /* EQ(dcp_db_write(db, &prof), IMM_SUCCESS); */
    imm_del(hmm);
    imm_del(state);

    dcp_normal_profile_del(prof0);
    dcp_normal_profile_del(prof1);
    imm_example1_deinit();
}
