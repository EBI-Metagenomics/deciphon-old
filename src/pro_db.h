#ifndef PRO_DB_H
#define PRO_DB_H

#include "db.h"
#include "entry_dist.h"
#include "meta.h"
#include "pro_cfg.h"
#include "pro_prof.h"
#include <stdio.h>

struct dcp_pro_db
{
    struct dcp_db super;
    struct imm_amino amino;
    struct imm_nuclt nuclt;
    struct imm_nuclt_code code;
    struct pro_prof prof;
};

extern struct dcp_pro_db const dcp_pro_db_default;

enum rc dcp_pro_db_setup_multi_readers(struct dcp_pro_db *db,
                                           unsigned nfiles, FILE *fp[]);

enum rc dcp_pro_db_openr(struct dcp_pro_db *db, FILE *restrict fp);

enum rc dcp_pro_db_openw(struct dcp_pro_db *db, FILE *restrict fp,
                             struct imm_amino const *amino,
                             struct imm_nuclt const *nuclt,
                             struct dcp_pro_cfg cfg);

enum rc dcp_pro_db_close(struct dcp_pro_db *db);

struct imm_amino const *dcp_pro_db_amino(struct dcp_pro_db const *db);

struct imm_nuclt const *dcp_pro_db_nuclt(struct dcp_pro_db const *db);

struct dcp_pro_cfg dcp_pro_db_cfg(struct dcp_pro_db const *db);

enum rc dcp_pro_db_read(struct dcp_pro_db *db, struct pro_prof *prof);

enum rc dcp_pro_db_write(struct dcp_pro_db *db,
                             struct pro_prof const *prof);

struct pro_prof *dcp_pro_db_profile(struct dcp_pro_db *db);

struct dcp_db *dcp_pro_db_super(struct dcp_pro_db *db);

#endif
