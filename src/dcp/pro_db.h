#ifndef DCP_PRO_DB_H
#define DCP_PRO_DB_H

#include "dcp/db.h"
#include "dcp/entry_dist.h"
#include "dcp/export.h"
#include "dcp/meta.h"
#include "dcp/pro_cfg.h"
#include "dcp/pro_prof.h"
#include <stdio.h>

struct dcp_pro_db
{
    struct dcp_db super;
    struct imm_amino amino;
    struct imm_nuclt nuclt;
    struct dcp_pro_prof prof;
};

DCP_API struct dcp_pro_db *dcp_pro_db_openr(FILE *restrict fd);

DCP_API struct dcp_pro_db *dcp_pro_db_openw(FILE *restrict fd,
                                            struct imm_amino const *amino,
                                            struct imm_nuclt const *nuclt,
                                            struct dcp_pro_cfg cfg);

DCP_API enum dcp_rc dcp_pro_db_close(struct dcp_pro_db *db);

DCP_API struct imm_amino const *dcp_pro_db_amino(struct dcp_pro_db const *db);

DCP_API struct imm_nuclt const *dcp_pro_db_nuclt(struct dcp_pro_db const *db);

DCP_API struct dcp_pro_cfg dcp_pro_db_cfg(struct dcp_pro_db const *db);

DCP_API enum dcp_rc dcp_pro_db_read(struct dcp_pro_db *db,
                                    struct dcp_pro_prof *prof);

DCP_API enum dcp_rc dcp_pro_db_write(struct dcp_pro_db *db,
                                     struct dcp_pro_prof const *prof);

DCP_API struct dcp_pro_prof *dcp_pro_db_profile(struct dcp_pro_db *db);

DCP_API struct dcp_db *dcp_pro_db_super(struct dcp_pro_db *db);

#endif
