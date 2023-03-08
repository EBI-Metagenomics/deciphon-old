#ifndef MODEL_SUMMARY_H
#define MODEL_SUMMARY_H

struct model_summary
{
  struct
  {
    struct imm_hmm const *hmm;
    struct imm_frame_state const *R;
  } null;

  struct
  {
    struct imm_hmm *hmm;
    struct imm_mute_state const *S;
    struct imm_frame_state const *N;
    struct imm_mute_state const *B;
    struct imm_mute_state const *E;
    struct imm_frame_state const *J;
    struct imm_frame_state const *C;
    struct imm_mute_state const *T;
  } alt;
};

#endif
