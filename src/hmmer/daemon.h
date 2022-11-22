#ifndef HMMER_DAEMON_H
#define HMMER_DAEMON_H

#include <stdbool.h>

int hmmerd_init(void);
int hmmerd_start(char const *hmm);
void hmmerd_stop(void);
int hmmerd_state(void);
char const *hmmerd_hmmfile(void);
void hmmerd_close(void);

#endif
