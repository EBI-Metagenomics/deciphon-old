#ifndef HMMER_RESULT_H
#define HMMER_RESULT_H

#include <stdio.h>

struct h3c_result;

struct hmmer_result
{
  struct h3c_result *handle;
};

int hmmer_result_init(struct hmmer_result *);
void hmmer_result_cleanup(struct hmmer_result *);

int hmmer_result_nhits(struct hmmer_result const *);
int hmmer_result_pack(struct hmmer_result const *, FILE *);

#endif
