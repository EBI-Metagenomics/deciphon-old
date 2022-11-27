#ifndef SCAN_PROD_HMMER_H
#define SCAN_PROD_HMMER_H

#include "deciphon_limits.h"
#include "sched_structs.h"

#if 0
struct h3c_result;

struct prod_hmmer
{
    long scan_id;
    long seq_id;
    char profile_name[SCHED_PROFILE_NAME_SIZE];
    char filename[FILENAME_SIZE];
};

void prod_hmmer_init(struct prod_hmmer *);
void prod_hmmer_set_scan_id(struct prod_hmmer *, long);
void prod_hmmer_set_seq_id(struct prod_hmmer *, long);
int prod_hmmer_set_profname(struct prod_hmmer *, char const *);
int prod_hmmer_write(struct prod_hmmer *, struct h3c_result *);
int prod_hmmer_read(struct prod_hmmer *, struct h3c_result *);
#endif

char const *prod_hmmer_filename(char *filename, long scan_id, long seq_id,
                                char const *profile_name);

#endif
