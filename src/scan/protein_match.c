#include "scan/protein_match.h"
#include "logy.h"
#include "model/protein_profile.h"
#include <string.h>

enum rc protein_match_init(struct protein_match *pm,
                           struct protein_profile const *prof,
                           struct match const *m)
{
    match_copy(&pm->match, m);

    struct imm_step const *step = pm->match.step;
    pm->mute = protein_state_is_mute(step->state_id);

    char state[IMM_STATE_NAME_SIZE] = {0};
    pm->match.profile->state_name(step->state_id, state);

    struct imm_seq const *f = pm->match.frag;

    if (!pm->mute)
    {
        pm->codon = imm_codon_any(prof->code->nuclt);
        if (protein_profile_decode(prof, f, step->state_id, &pm->codon))
            return einval("could not decode into codon");

        pm->amino = imm_gc_decode(1, pm->codon);
    }
    return 0;
}

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

static void as_string(struct protein_match const *pm, char *buf)
{
    memcpy(buf, pm->match.frag->str, pm->match.frag->size);
    buf += pm->match.frag->size;
    *buf++ = ',';

    struct imm_step const *step = pm->match.step;
    pm->match.profile->state_name(step->state_id, buf);
    buf += strlen(buf);
    *buf++ = ',';

    if (!pm->mute)
    {
        *buf++ = imm_codon_asym(&pm->codon);
        *buf++ = imm_codon_bsym(&pm->codon);
        *buf++ = imm_codon_csym(&pm->codon);
    }

    *buf++ = ',';

    if (!pm->mute) *buf++ = imm_gc_decode(1, pm->codon);

    *buf = '\0';
}

enum rc protein_match_write(FILE *fp, struct match const *m)
{
    struct protein_match pm = {0};
    struct protein_profile const *prof =
        (struct protein_profile const *)m->profile;

    enum rc rc = protein_match_init(&pm, prof, m);
    if (rc) return rc;

    char buf[IMM_STATE_NAME_SIZE + 20] = {0};
    as_string(&pm, buf);

    if (fprintf(fp, "%s", buf) < 0) return eio("failed to write match");

    return 0;
}
