#include "protein_codec.h"
#include "imm/imm.h"
#include "protein_profile.h"
#include "protein_state.h"

enum rc dcp_protein_codec_next(struct dcp_protein_codec *codec,
                               struct imm_seq const *seq,
                               struct imm_codon *codon)
{
    struct imm_step const *step = NULL;

    for (; codec->idx < imm_path_nsteps(codec->path); codec->idx++)
    {
        step = imm_path_step(codec->path, codec->idx);
        if (!dcp_protein_state_is_mute(step->state_id)) break;
    }

    if (codec->idx >= imm_path_nsteps(codec->path)) return END;

    unsigned size = step->seqlen;
    struct imm_seq frag = imm_subseq(seq, codec->start, size);
    codec->start += size;
    codec->idx++;
    return dcp_protein_prof_decode(codec->prof, &frag, step->state_id, codon);
}
