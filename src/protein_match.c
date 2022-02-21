#include "protein_match.h"
#include "common/logger.h"
#include "profile.h"
#include "protein_profile.h"
#include "common/rc.h"

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

enum rc protein_match_write_cb(FILE *fp, void const *match)
{
    struct match const *m = (struct match const *)match;
    struct protein_profile const *prof =
        (struct protein_profile const *)m->profile;
    struct imm_step const *step = m->step;
    struct imm_codon codon = imm_codon_any(prof->code->nuclt);

    char state[IMM_STATE_NAME_SIZE] = {0};
    m->profile->state_name(step->state_id, state);

    struct imm_seq const *f = m->frag;

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

    return DCP_OK;

cleanup:
    error(RC_EIO, "failed to write match");
    return DCP_EFAIL;
}
