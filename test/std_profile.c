#include "dcp/dcp.h"
#include "hope/hope.h"
#include "imm/imm.h"

void test_std_profile(void);

int main(void)
{
    test_std_profile();
    return hope_status();
}

void test_std_profile(void)
{
    imm_example1_init();
    struct imm_example1 *m = &imm_example1;

    /* Profile 0 */
    struct dcp_meta mt0 = dcp_meta("NAME0", "ACC0");
    struct dcp_std_profile *p0 = dcp_std_profile_new(&m->abc, mt0);
    imm_hmm_reset_dp(&m->null.hmm, imm_super(&m->null.n), &p0->dp.null);
    imm_hmm_reset_dp(&m->hmm, imm_super(&m->end), &p0->dp.alt);

    /* Profile 1 */
    struct imm_mute_state state;
    imm_mute_state_init(&state, 3, &m->abc);
    struct imm_hmm hmm;
    imm_hmm_init(&hmm, &m->abc);
    EQ(imm_hmm_add_state(&hmm, imm_super(&state)), IMM_SUCCESS);
    EQ(imm_hmm_set_start(&hmm, imm_super(&state), imm_log(0.3)), IMM_SUCCESS);
    struct dcp_meta mt1 = dcp_meta("NAME1", "ACC1");
    struct dcp_std_profile *p1 = dcp_std_profile_new(&m->abc, mt1);
    imm_hmm_reset_dp(&m->null.hmm, imm_super(&state), &p1->dp.null);
    imm_hmm_reset_dp(&m->hmm, imm_super(&state), &p1->dp.alt);

    dcp_std_profile_del(p0);
    dcp_std_profile_del(p1);
}
