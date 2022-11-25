#include "scan/prot_result.h"
#include "die.h"
#include "hmmer/client.h"
#include "logy.h"
#include "loop/now.h"
#include "scan/prot_match.h"
#include "scan/prot_match_iter.h"
#include "sched_structs.h"
#include <stddef.h>

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
}

void prot_result_query_hmmer3(struct prot_result *r, int prof_idx)
{
    int rc = hmmerc_put(0, prof_idx, r->amino_seq, now() + 5000);
    if (rc) die();
    double ln_evalue = 0;
    rc = hmmerc_pop(0, &ln_evalue);
    if (rc) die();
}
