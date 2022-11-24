#include "scan/protein_match.h"
#include "logy.h"
#include "model/protein_profile.h"
#include <string.h>

void protein_match_init(struct protein_match *pm,
                        struct protein_profile const *prof)
{
    match_init(&pm->match, (struct profile const *)prof);
    pm->mute = false;
    pm->codon = imm_codon_any(prof->code->nuclt);
    pm->amino = '\0';
}

enum rc protein_match_setup(struct protein_match *pm,
                            struct imm_step const *step,
                            struct imm_seq const *frag)
{
    match_setup(&pm->match, step, frag);
    pm->mute = protein_state_is_mute(step->state_id);
    if (!pm->mute)
    {
        struct protein_profile const *prof =
            (struct protein_profile const *)pm->match.profile;

        pm->codon = imm_codon_any(prof->code->nuclt);
        if (protein_profile_decode(prof, frag, step->state_id, &pm->codon))
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

static void as_string(struct protein_match const *pm, char *buf);

enum rc protein_match_write(struct protein_match const *pm, FILE *fp)
{
    char buf[IMM_STATE_NAME_SIZE + 20] = {0};
    as_string(pm, buf);
    return fprintf(fp, "%s", buf) < 0 ? eio("failed to write match") : 0;
}

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
