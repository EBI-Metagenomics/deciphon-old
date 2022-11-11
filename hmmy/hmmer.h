#ifndef HMMY_HMMER_H
#define HMMY_HMMER_H

#include <stdbool.h>

enum hmmer_state
{
    HMMER_OFF,
    HMMER_BOOT,
    HMMER_READY,
    HMMER_RUN,
    HMMER_DONE,
    HMMER_FAIL,
};

void hmmer_init(char const *podman);
void hmmer_start(char const *hmm_file);
void hmmer_stop(void);
bool hmmer_offline(void);
enum hmmer_state hmmer_state(void);
char const *hmmer_hmmfile(void);
void hmmer_cleanup(void);

#endif
