#ifndef HMMER_CLIENT_H
#define HMMER_CLIENT_H

struct hmmer_result;

int hmmer_client_deadline(long timeout);
int hmmer_client_start(int num_streams, long deadline);
int hmmer_client_put(int id, int hmm_idx, char const *seq, long deadline);
int hmmer_client_pop(int id, struct hmmer_result *);
void hmmer_client_stop(void);

#endif
