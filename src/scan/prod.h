#ifndef SCAN_PROD_H
#define SCAN_PROD_H

#include "core/limits.h"
#include "sched_structs.h"
#include <stdint.h>
#include <stdio.h>

struct imm_path;
struct imm_seq;
struct match;

enum imm_abc_typeid;
enum profile_typeid;

struct prod
{
    int64_t id;

    int64_t scan_id;
    int64_t seq_id;

    char profile_name[SCHED_PROFILE_NAME_SIZE];
    char abc_name[SCHED_ABC_NAME_SIZE];

    double alt_loglik;
    double null_loglik;

    char profile_typeid[SCHED_PROFILE_TYPEID_SIZE];
    char version[SCHED_VERSION_SIZE];

    char match[SCHED_MATCH_SIZE];
};

typedef enum rc prod_fwrite_match_fn_t(FILE *fp, void const *match);

enum rc prod_fwrite(struct prod const *prod, struct imm_seq const *seq,
                    struct imm_path const *path, unsigned thread_num,
                    prod_fwrite_match_fn_t *, struct match *match);

enum rc prod_fopen(unsigned nthreads);
void prod_setup_job(struct prod *prod, char const *abc_name,
                    char const *prof_typeid, int64_t scan_id);
void prod_setup_seq(struct prod *prod, int64_t seq_id);
void prod_fcleanup(void);
enum rc prod_fclose(void);

FILE *prod_final_fp(void);
char const *prod_final_path(void);
void prod_final_cleanup(void);

#endif
