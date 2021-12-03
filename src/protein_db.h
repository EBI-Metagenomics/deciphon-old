#ifndef PROTEIN_DB_H
#define PROTEIN_DB_H

#include "db.h"
#include "entry_dist.h"
#include "meta.h"
#include "protein_cfg.h"
#include "protein_profile.h"
#include <stdio.h>

struct dcp_protein_db
{
    struct dcp_db super;
    struct imm_amino amino;
    struct imm_nuclt nuclt;
    struct imm_nuclt_code code;
    struct protein_prof prof;
};

extern struct dcp_protein_db const dcp_protein_db_default;

enum rc dcp_protein_db_setup_multi_readers(struct dcp_protein_db *db,
                                           unsigned nfiles, FILE *fp[]);

enum rc dcp_protein_db_openr(struct dcp_protein_db *db, FILE *restrict fp);

enum rc dcp_protein_db_openw(struct dcp_protein_db *db, FILE *restrict fp,
                             struct imm_amino const *amino,
                             struct imm_nuclt const *nuclt,
                             struct dcp_protein_cfg cfg);

enum rc dcp_protein_db_close(struct dcp_protein_db *db);

struct imm_amino const *dcp_protein_db_amino(struct dcp_protein_db const *db);

struct imm_nuclt const *dcp_protein_db_nuclt(struct dcp_protein_db const *db);

struct dcp_protein_cfg dcp_protein_db_cfg(struct dcp_protein_db const *db);

enum rc dcp_protein_db_read(struct dcp_protein_db *db, struct protein_prof *prof);

enum rc dcp_protein_db_write(struct dcp_protein_db *db,
                             struct protein_prof const *prof);

struct protein_prof *dcp_protein_db_profile(struct dcp_protein_db *db);

struct dcp_db *dcp_protein_db_super(struct dcp_protein_db *db);

#endif
