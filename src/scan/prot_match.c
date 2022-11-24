#include "scan/prot_match.h"
#include "logy.h"
#include "model/prot_profile.h"
#include <string.h>

void prot_match_init(struct prot_match *pm, struct prot_profile const *prof)
{
    match_init(&pm->match, (struct profile const *)prof);
    pm->mute = false;
    pm->codon = imm_codon_any(prof->code->nuclt);
    pm->amino = '\0';
}

enum rc prot_match_setup(struct prot_match *pm, struct imm_step step,
                         struct imm_seq frag)
{
    match_setup(&pm->match, step, frag);
    pm->mute = prot_state_is_mute(step.state_id);
    if (!pm->mute)
    {
        struct prot_profile const *prof =
            (struct prot_profile const *)pm->match.profile;

        pm->codon = imm_codon_any(prof->code->nuclt);
        if (prot_profile_decode(prof, &frag, step.state_id, &pm->codon))
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

static void as_string(struct prot_match const *pm, char *buf);

enum rc prot_match_write(struct prot_match const *pm, FILE *fp)
{
    char buf[IMM_STATE_NAME_SIZE + 20] = {0};
    as_string(pm, buf);
    return fprintf(fp, "%s", buf) < 0 ? eio("failed to write match") : 0;
}

static void as_string(struct prot_match const *pm, char *buf)
{
    memcpy(buf, pm->match.frag.str, pm->match.frag.size);
    buf += pm->match.frag.size;
    *buf++ = ',';

    pm->match.profile->state_name(pm->match.step.state_id, buf);
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
