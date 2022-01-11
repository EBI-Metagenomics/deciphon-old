#ifndef STMT_H
#define STMT_H

#include "common/rc.h"

enum stmt
{
    DB_INSERT,
    DB_SELECT_BY_ID,
    DB_SELECT_BY_XXH64,
    JOB_INSERT,
    JOB_GET_PEND,
    JOB_GET_STATE,
    JOB_SELECT,
    JOB_SET_RUN,
    JOB_SET_ERROR,
    JOB_SET_DONE,
    PROD_INSERT,
    PROD_SELECT,
    PROD_SELECT_NEXT,
    SEQ_INSERT,
    SEQ_SELECT,
    SEQ_SELECT_NEXT
};

struct sqlite3_stmt;
extern struct sqlite3_stmt *stmt[];

enum rc stmt_init(void);
void stmt_del(void);

#endif
