#include "protein_match.h"
#include "dcp_sched/sched.h"
#include "logger.h"
#include "profile.h"
#include "protein_profile.h"
#include "rc.h"

/* Match example
 *             ___________________________
 *             |   match0   |   match1   |
 *             ---------------------------
 * Output----->| CG,M1,CGA,K;CG,M4,CGA,K |
 *             ---|-|---|--|--------------
 * -----------   /  |   |  \    ---------------
 * | matched |__/   |   |   \___| most likely |
 * | letters |      |   |       | amino acid  |
 * -----------      |   |       ---------------
 *      -------------   ---------------
 *      | hmm state |   | most likely |
 *      -------------   | codon       |
 *                      ---------------
 */

int protein_match_write_cb(FILE *fp, void const *match)
{
    struct protein_match const *m = (struct protein_match const *)match;
    struct protein_profile const *prof = m->profile;
    struct imm_step const *step = m->match.step;
    struct imm_codon codon = imm_codon_any(m->profile->code->nuclt);

    char state[IMM_STATE_NAME_SIZE] = {0};
    m->match.profile->state_name(step->state_id, state);

    struct imm_seq const *f = m->match.frag;

    char ccodon[4] = {0};
    char camino[2] = {0};
    if (!protein_state_is_mute(step->state_id))
    {
        if (protein_profile_decode(prof, f, step->state_id, &codon))
            goto cleanup;

        ccodon[0] = imm_codon_asym(&codon);
        ccodon[1] = imm_codon_bsym(&codon);
        ccodon[2] = imm_codon_csym(&codon);
        camino[0] = imm_gc_decode(1, codon);
    }

    if (fprintf(fp, "%.*s,", f->size, f->str) < 0) goto cleanup;
    if (fprintf(fp, "%s,", state) < 0) goto cleanup;
    if (fprintf(fp, "%s,", ccodon) < 0) goto cleanup;
    if (fprintf(fp, "%s", camino) < 0) goto cleanup;

    return SCHED_DONE;

cleanup:
    error(RC_IOERROR, "failed to write match");
    return SCHED_FAIL;
}
