#ifndef DCP_PROTEIN_DB_H
#define DCP_PROTEIN_DB_H

#include "db.h"
#include "entry_dist.h"
#include "metadata.h"
#include "protein_cfg.h"
#include "protein_profile.h"
#include <stdio.h>

struct protein_db
{
    struct db super;

    struct imm_amino amino;
    struct imm_nuclt nuclt;
    struct imm_nuclt_code code;
    struct protein_cfg cfg;
};

enum rc protein_db_openr(struct protein_db *db, FILE *fp);

enum rc protein_db_openw(struct protein_db *db, FILE *fp,
                         struct imm_amino const *amino,
                         struct imm_nuclt const *nuclt, struct protein_cfg cfg);

struct imm_amino const *protein_db_amino(struct protein_db const *db);

struct imm_nuclt const *protein_db_nuclt(struct protein_db const *db);

struct protein_cfg protein_db_cfg(struct protein_db const *db);

struct db *protein_db_super(struct protein_db *db);

#endif
