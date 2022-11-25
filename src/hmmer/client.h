#ifndef HMMER_CLIENT_H
#define HMMER_CLIENT_H

int hmmerc_start(int num_streams, long deadline);
int hmmerc_put(int id, int hmm_idx, char const *seq, long deadline);
int hmmerc_pop(int id, double *ln_evalue);
void hmmerc_stop(void);

#endif
