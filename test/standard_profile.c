#include "model/standard_profile.h"
#include "hope.h"
#include "imm/imm.h"

int main(void)
{
    imm_example1_init(IMM_EXAMPLE1_SIZE);

    struct imm_abc const *abc = &imm_example1.abc;
    struct imm_code const *code = &imm_example1.code;
    struct imm_hmm *null = &imm_example1.null.hmm;
    struct imm_hmm *alt = &imm_example1.hmm;

    struct imm_mute_state state0;
    imm_mute_state_init(&state0, 3, abc);
    eq(imm_hmm_add_state(null, imm_super(&state0)), IMM_OK);
    eq(imm_hmm_set_start(null, imm_super(&state0), imm_log(0.3)), IMM_OK);

    imm_float lprobs[IMM_EXAMPLE1_NUCLT_SIZE] = {0};
    struct imm_normal_state state1;
    imm_normal_state_init(&state1, 1, abc, lprobs);
    eq(imm_hmm_add_state(alt, imm_super(&state1)), IMM_OK);
    eq(imm_hmm_set_start(alt, imm_super(&state1), imm_log(0.1)), IMM_OK);

    struct standard_profile prof;
    standard_profile_init(&prof, "accession", code);
    eq(imm_hmm_reset_dp(null, imm_super(&state0), &prof.dp.null), IMM_OK);
    eq(imm_hmm_reset_dp(alt, imm_super(&state1), &prof.dp.alt), IMM_OK);

    profile_del((struct profile *)&prof);
    return hope_status();
}
