#ifndef DCP_STD_DB_H
#define DCP_STD_DB_H

#include "db.h"
#include "meta.h"
#include "std_prof.h"
#include <stdio.h>

struct dcp_std_db
{
    struct dcp_db super;
    struct imm_abc abc;
    struct imm_code code;
    struct dcp_std_prof prof;
};

void dcp_std_db_init(struct dcp_std_db *db);

enum dcp_rc dcp_std_db_openr(struct dcp_std_db *db, FILE *restrict fd);

enum dcp_rc dcp_std_db_openw(struct dcp_std_db *db, FILE *restrict fd,
                             struct imm_code const *code);

enum dcp_rc dcp_std_db_close(struct dcp_std_db *db);

struct imm_abc const *dcp_std_db_abc(struct dcp_std_db const *db);

enum dcp_rc dcp_std_db_read(struct dcp_std_db *db, struct dcp_std_prof *prof);

enum dcp_rc dcp_std_db_write(struct dcp_std_db *db,
                             struct dcp_std_prof const *prof);

struct dcp_std_prof *dcp_std_db_profile(struct dcp_std_db *db);

struct dcp_db *dcp_std_db_super(struct dcp_std_db *db);

#endif
