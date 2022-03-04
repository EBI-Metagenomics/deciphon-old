#ifndef DECIPHON_PROD_H
#define DECIPHON_PROD_H

#include "deciphon/limits.h"
#include "deciphon/util/util.h"
#include <stdint.h>
#include <stdio.h>

struct imm_path;
struct imm_seq;
struct match;

struct prod
{
    int64_t id;

    int64_t job_id;
    int64_t seq_id;

    char profile_name[PROFILE_NAME_SIZE];
    char abc_name[ABC_NAME_SIZE];

    double alt_loglik;
    double null_loglik;

    char profile_typeid[PROFILE_TYPEID_SIZE];
    char version[VERSION_SIZE];

    char match[MATCH_SIZE];
};

typedef enum rc(prod_fwrite_match_cb)(FILE *fp, void const *match);

enum rc prod_fwrite(struct prod const *prod, struct imm_seq const *seq,
                    struct imm_path const *path, unsigned partition,
                    prod_fwrite_match_cb fwrite_match, struct match *match);

enum rc prod_fopen(unsigned num_threads);
enum rc prod_fclose(void);

#endif
