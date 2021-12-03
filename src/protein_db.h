#ifndef PROTEIN_DB_H
#define PROTEIN_DB_H

#include "db.h"
#include "entry_dist.h"
#include "meta.h"
#include "protein_cfg.h"
#include "protein_profile.h"
#include <stdio.h>

struct protein_db
{
    struct dcp_db super;
    struct imm_amino amino;
    struct imm_nuclt nuclt;
    struct imm_nuclt_code code;
    struct protein_profile prof;
};

extern struct protein_db const protein_db_default;

enum rc protein_db_setup_multi_readers(struct protein_db *db,
                                           unsigned nfiles, FILE *fp[]);

enum rc protein_db_openr(struct protein_db *db, FILE *restrict fp);

enum rc protein_db_openw(struct protein_db *db, FILE *restrict fp,
                             struct imm_amino const *amino,
                             struct imm_nuclt const *nuclt,
                             struct protein_cfg cfg);

enum rc protein_db_close(struct protein_db *db);

struct imm_amino const *protein_db_amino(struct protein_db const *db);

struct imm_nuclt const *protein_db_nuclt(struct protein_db const *db);

struct protein_cfg protein_db_cfg(struct protein_db const *db);

enum rc protein_db_read(struct protein_db *db, struct protein_profile *prof);

enum rc protein_db_write(struct protein_db *db,
                             struct protein_profile const *prof);

struct protein_profile *protein_db_profile(struct protein_db *db);

struct dcp_db *protein_db_super(struct protein_db *db);

#endif
