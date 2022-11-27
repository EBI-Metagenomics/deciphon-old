#ifndef SCAN_PROD_H
#define SCAN_PROD_H

#include "deciphon_limits.h"
#include "sched_structs.h"
#include <stdio.h>

struct imm_path;
struct imm_seq;
struct match;

enum profile_typeid;

struct prod
{
    long id;

    long scan_id;
    long seq_id;

    char profile_name[SCHED_PROFILE_NAME_SIZE];
    char abc_name[SCHED_ABC_NAME_SIZE];

    double alt_loglik;
    double null_loglik;
    double evalue_log;

    char profile_typeid[SCHED_PROFILE_TYPEID_SIZE];
    char version[SCHED_VERSION_SIZE];

    char match[SCHED_MATCH_SIZE];
};

typedef enum rc prod_fwrite_match_fn_t(FILE *, struct match const *);

void prod_setup_job(struct prod *prod, char const *abc_name,
                    char const *prof_typeid, long scan_id);
void prod_setup_seq(struct prod *prod, long seq_id);

int prod_write_begin(struct prod const *prod, FILE *fp);
int prod_write_sep(FILE *fp);
int prod_write_end(FILE *fp);

#endif
