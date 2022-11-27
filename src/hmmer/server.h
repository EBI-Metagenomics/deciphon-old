#ifndef HMMER_SERVER_H
#define HMMER_SERVER_H

#include <stdbool.h>

int hmmer_server_start(char const *hmm);
void hmmer_server_stop(void);
int hmmer_server_state(void);
char const *hmmer_server_hmmfile(void);
void hmmer_server_close(void);

#endif
