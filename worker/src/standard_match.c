#include "standard_match.h"
#include "sched/sched.h"
#include "imm/imm.h"
#include "common/logger.h"
#include "profile.h"
#include "standard_state.h"

int standard_match_write_cb(FILE *fp, void const *match)
{
    char state[IMM_STATE_NAME_SIZE] = {0};
    struct standard_match const *m = (struct standard_match const *)match;
    struct imm_step const *step = m->match.step;

    m->match.profile->state_name(step->state_id, state);

    struct imm_seq const *f = m->match.frag;

    if (fprintf(fp, "%.*s,", f->size, f->str) < 0) goto cleanup;
    if (fprintf(fp, "%s", state) < 0) goto cleanup;

    return RC_DONE;

cleanup:
    error(RC_EIO, "failed to write match");
    return RC_EFAIL;
}
