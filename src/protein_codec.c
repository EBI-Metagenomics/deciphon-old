#include "protein_codec.h"
#include "imm/imm.h"
#include "protein_profile.h"
#include "protein_state.h"

enum rc protein_codec_next(struct protein_codec *codec,
                               struct imm_seq const *seq,
                               struct imm_codon *codon)
{
    struct imm_step const *step = NULL;

    for (; codec->idx < imm_path_nsteps(codec->path); codec->idx++)
    {
        step = imm_path_step(codec->path, codec->idx);
        if (!protein_state_is_mute(step->state_id)) break;
    }

    if (codec->idx >= imm_path_nsteps(codec->path)) return END;

    unsigned size = step->seqlen;
    struct imm_seq frag = imm_subseq(seq, codec->start, size);
    codec->start += size;
    codec->idx++;
    return protein_prof_decode(codec->prof, &frag, step->state_id, codon);
}
