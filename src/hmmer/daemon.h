#ifndef HMMER_DAEMON_H
#define HMMER_DAEMON_H

#include <stdbool.h>

int hmmerd_init(void);
int hmmerd_start(char const *hmm);
void hmmerd_wait(long deadline);
void hmmerd_stop(void);
bool hmmerd_off(void);
bool hmmerd_on(void);
void hmmerd_close(void);

#endif
