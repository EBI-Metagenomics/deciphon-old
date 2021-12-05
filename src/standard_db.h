#ifndef STANDARD_DB_H
#define STANDARD_DB_H

#include "db.h"
#include "meta.h"
#include "standard_profile.h"
#include <stdio.h>

struct standard_db
{
    struct db super;
    struct imm_abc abc;
    struct imm_code code;
    struct standard_profile prof;
};

void standard_db_init(struct standard_db *db);

enum rc standard_db_openr(struct standard_db *db, FILE *restrict fd);

enum rc standard_db_openw(struct standard_db *db, FILE *restrict fd,
                              struct imm_code const *code);

enum rc standard_db_close(struct standard_db *db);

struct imm_abc const *standard_db_abc(struct standard_db const *db);

enum rc standard_db_read(struct standard_db *db,
                             struct standard_profile *prof);

enum rc standard_db_write(struct standard_db *db,
                              struct standard_profile const *prof);

struct standard_profile *standard_db_profile(struct standard_db *db);

struct db *standard_db_super(struct standard_db *db);

#endif
