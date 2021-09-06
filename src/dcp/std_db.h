#ifndef DCP_STD_DB_H
#define DCP_STD_DB_H

#include "dcp/export.h"
#include "dcp/meta.h"
#include <stdio.h>

struct dcp_db;
struct dcp_std_db;
struct dcp_std_prof;
struct imm_abc;

DCP_API struct dcp_std_db *dcp_std_db_openr(FILE *restrict fd);

DCP_API struct dcp_std_db *dcp_std_db_openw(FILE *restrict fd,
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
