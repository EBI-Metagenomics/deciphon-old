#ifndef HMMER_RESULT_H
#define HMMER_RESULT_H

#include <stdio.h>

struct hmmer_result;
struct h3c_result;

int hmmer_result_new(struct hmmer_result **);
void hmmer_result_del(struct hmmer_result const *);

unsigned hmmer_result_nhits(struct hmmer_result const *);
double hmmer_result_evalue_ln(struct hmmer_result const *);
double hmmer_result_evalue(struct hmmer_result const *);

int hmmer_result_write(struct hmmer_result *, FILE *);
int hmmer_result_read(struct hmmer_result *, FILE *);

struct h3c_result *hmmer_result_handle(struct hmmer_result *);

#endif
