#ifndef DCP_PRO_DB_H
#define DCP_PRO_DB_H

#include "dcp/export.h"
#include "dcp/meta.h"
#include "dcp/pro_cfg.h"
#include <stdio.h>

struct dcp_pro_db;
struct dcp_pro_prof;
struct imm_amino;
struct imm_nuclt;

DCP_API struct dcp_pro_db *dcp_pro_db_openr(FILE *restrict fd);

DCP_API struct dcp_pro_db *dcp_pro_db_openw(FILE *restrict fd,
                                            struct dcp_pro_cfg cfg);

DCP_API enum dcp_rc dcp_pro_db_close(struct dcp_pro_db *db);

DCP_API struct imm_amino const *dcp_pro_db_amino(struct dcp_pro_db const *db);

DCP_API struct imm_nuclt const *dcp_pro_db_nuclt(struct dcp_pro_db const *db);

DCP_API enum dcp_rc dcp_pro_db_read(struct dcp_pro_db *db,
                                    struct dcp_pro_prof *prof);

DCP_API enum dcp_rc dcp_pro_db_write(struct dcp_pro_db *db,
                                     struct dcp_pro_prof const *prof);

DCP_API struct dcp_pro_prof *dcp_pro_db_profile(struct dcp_pro_db *db);

DCP_API unsigned dcp_pro_db_nprofiles(struct dcp_pro_db const *db);

DCP_API struct dcp_meta dcp_pro_db_meta(struct dcp_pro_db const *db,
                                        unsigned idx);

DCP_API bool dcp_pro_db_end(struct dcp_pro_db const *db);

#endif