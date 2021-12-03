#ifndef DCP_STD_DB_H
#define DCP_STD_DB_H

#include "db.h"
#include "meta.h"
#include "standard_profile.h"
#include <stdio.h>

struct dcp_standard_db
{
    struct dcp_db super;
    struct imm_abc abc;
    struct imm_code code;
    struct standard_profile prof;
};

void dcp_standard_db_init(struct dcp_standard_db *db);

enum rc dcp_standard_db_openr(struct dcp_standard_db *db, FILE *restrict fd);

enum rc dcp_standard_db_openw(struct dcp_standard_db *db, FILE *restrict fd,
                         struct imm_code const *code);

enum rc dcp_standard_db_close(struct dcp_standard_db *db);

struct imm_abc const *dcp_standard_db_abc(struct dcp_standard_db const *db);

enum rc dcp_standard_db_read(struct dcp_standard_db *db, struct standard_profile *prof);

enum rc dcp_standard_db_write(struct dcp_standard_db *db,
                         struct standard_profile const *prof);

struct standard_profile *dcp_standard_db_profile(struct dcp_standard_db *db);

struct dcp_db *dcp_standard_db_super(struct dcp_standard_db *db);

#endif
