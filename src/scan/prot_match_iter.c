#include "scan/prot_match_iter.h"
#include "logy.h"
#include "model/prot_model.h"

void prot_match_iter(struct prot_match_iter *iter, struct prot_result const *r)
{
    iter->result = r;
    prot_match_init(&iter->match, r->profile);
    iter->idx = 0;
    iter->offset = 0;
}

struct prot_match const *prot_match_iter_next(struct prot_match_iter *iter)
{
    if (iter->idx >= imm_path_nsteps(iter->result->path)) return NULL;

    struct imm_path const *path = iter->result->path;
    struct imm_seq const *seq = iter->result->seq;

    struct imm_step const *step = imm_path_step(path, iter->idx);
    struct imm_seq frag = imm_subseq(seq, iter->offset, step->seqlen);

    iter->match.mute = prot_state_is_mute(step->state_id);
    if (prot_match_setup(&iter->match, *step, frag))
    {
        einval("bug in the match setup");
        return NULL;
    }

    iter->idx += 1;
    iter->offset += step->seqlen;
    return &iter->match;
}
