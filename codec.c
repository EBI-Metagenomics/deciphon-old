#include "codec.h"
#include "imm/imm.h"
#include "protein.h"
#include "state.h"

struct codec codec_init(struct protein const *protein,
                        struct imm_path const *path)
{
  return (struct codec){0, 0, protein, path};
}

int codec_next(struct codec *codec, struct imm_seq const *seq,
               struct imm_codon *codon)
{
  struct imm_step const *step = NULL;

  for (; codec->idx < imm_path_nsteps(codec->path); codec->idx++)
  {
    step = imm_path_step(codec->path, codec->idx);
    if (!state_is_mute(step->state_id)) break;
  }

  if (codec_end(codec)) return 0;

  unsigned size = step->seqlen;
  struct imm_seq frag = imm_subseq(seq, codec->start, size);
  codec->start += size;
  codec->idx++;
  return protein_decode(codec->protein, &frag, step->state_id, codon);
}

bool codec_end(struct codec const *codec)
{
  return codec->idx >= imm_path_nsteps(codec->path);
}
