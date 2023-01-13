#include "prot_codec.h"
#include "imm/imm.h"
#include "prot_prof.h"
#include "prot_state.h"

struct prot_codec prot_codec_init(struct prot_prof const *prof,
                                  struct imm_path const *path)
{
  return (struct prot_codec){0, 0, prof, path};
}

int prot_codec_next(struct prot_codec *codec, struct imm_seq const *seq,
                    struct imm_codon *codon)
{
  struct imm_step const *step = NULL;

  for (; codec->idx < imm_path_nsteps(codec->path); codec->idx++)
  {
    step = imm_path_step(codec->path, codec->idx);
    if (!prot_state_is_mute(step->state_id)) break;
  }

  if (prot_codec_end(codec)) return 0;

  unsigned size = step->seqlen;
  struct imm_seq frag = imm_subseq(seq, codec->start, size);
  codec->start += size;
  codec->idx++;
  return prot_prof_decode(codec->prof, &frag, step->state_id, codon);
}

bool prot_codec_end(struct prot_codec const *codec)
{
  return codec->idx >= imm_path_nsteps(codec->path);
}
