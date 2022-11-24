#include "scan/prot_result.h"
#include "logy.h"
#include "scan/prot_match.h"
#include "scan/prot_match_iter.h"
#include "sched_structs.h"
#include <stddef.h>

static int infer_amino_sequence(struct prot_result *r);

void prot_result_init(struct prot_result *r, struct prot_profile const *prof,
                      struct imm_seq const *seq, struct imm_path const *path)
{
    r->profile = prof;
    r->seq = seq;
    r->path = path;
    prot_match_init(&r->match, prof);
    r->amino_seq[0] = '\0';

    struct prot_match_iter iter = {0};
    prot_match_iter(&iter, r);

    struct prot_match const *match = NULL;
    char *aseq = r->amino_seq;

    while ((match = prot_match_iter_next(&iter)))
    {
        if (!match->mute) *aseq++ = match->amino;
    }

    *aseq = '\0';

    if (infer_amino_sequence(r)) einval("bug in the match setup");
}

int prot_result_ask(struct prot_result *r)
{
    int rc = infer_amino_sequence(r);
    if (rc) return rc;
    return rc;
}

static int infer_amino_sequence(struct prot_result *r)
{
    char *aseq = r->amino_seq;
    unsigned start = 0;
    for (unsigned idx = 0; idx < imm_path_nsteps(r->path); idx++)
    {
        struct imm_step const *step = imm_path_step(r->path, idx);
        struct imm_seq frag = imm_subseq(r->seq, start, step->seqlen);
        int rc = prot_match_setup(&r->match, *step, frag);
        if (rc) return rc;

        *aseq++ = r->match.amino;

        start += step->seqlen;
    }
    *aseq = '\0';
    return 0;
}
