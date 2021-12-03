#include "pro_codec.h"
#include "imm/imm.h"
#include "pro_prof.h"
#include "pro_state.h"

enum dcp_rc dcp_pro_codec_next(struct dcp_pro_codec *codec,
                               struct imm_seq const *seq,
                               struct imm_codon *codon)
{
    struct imm_step const *step = NULL;

    for (; codec->idx < imm_path_nsteps(codec->path); codec->idx++)
    {
        step = imm_path_step(codec->path, codec->idx);
        if (!dcp_pro_state_is_mute(step->state_id)) break;
    }

    if (codec->idx >= imm_path_nsteps(codec->path)) return DCP_END;

    unsigned size = step->seqlen;
    struct imm_seq frag = imm_subseq(seq, codec->start, size);
    codec->start += size;
    codec->idx++;
    return dcp_pro_prof_decode(codec->prof, &frag, step->state_id, codon);
}
