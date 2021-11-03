#ifndef PROD_H
#define PROD_H

#include "dcp/rc.h"
#include <stdio.h>

struct prod
{
    unsigned match_id;
    char seq_id[32];
    char prof_id[32];
    unsigned start;
    unsigned end;
    char abc_id[16];
    double loglik;
    double null_loglik;
    char model[5];
    char version[8];
    char db_id[32];
    char seq_hash[511];
};

void prod_init(struct prod *p, unsigned match_id, char const seq_id[static 1],
               char const prof_id[static 1], unsigned start, unsigned end,
               char const abc[static 1], double loglik, double null_loglik,
               char const model[static 1], char const version[static 1],
               char const db_id[static 1], char const seq_hash[static 1]);

enum dcp_rc prod_write(struct prod *p, FILE *restrict fd);

#endif
