#ifndef DCP_STD_DB_H
#define DCP_STD_DB_H

#include "dcp/db.h"
#include "dcp/export.h"
#include "dcp/meta.h"
#include "dcp/std_prof.h"
#include <stdio.h>

struct imm_abc;

struct dcp_std_db
{
    struct dcp_db super;
    struct imm_abc abc;
    struct dcp_std_prof prof;
};

DCP_API void dcp_std_db_init(struct dcp_std_db *db);

DCP_API enum dcp_rc dcp_std_db_openr(struct dcp_std_db *db, FILE *restrict fd);

DCP_API enum dcp_rc dcp_std_db_openw(struct dcp_std_db *db, FILE *restrict fd,
                                     struct imm_abc const *abc);

DCP_API enum dcp_rc dcp_std_db_close(struct dcp_std_db *db);

DCP_API struct imm_abc const *dcp_std_db_abc(struct dcp_std_db const *db);

DCP_API enum dcp_rc dcp_std_db_read(struct dcp_std_db *db,
                                    struct dcp_std_prof *prof);

DCP_API enum dcp_rc dcp_std_db_write(struct dcp_std_db *db,
                                     struct dcp_std_prof const *prof);

DCP_API struct dcp_std_prof *dcp_std_db_profile(struct dcp_std_db *db);

DCP_API struct dcp_db *dcp_std_db_super(struct dcp_std_db *db);

#endif
