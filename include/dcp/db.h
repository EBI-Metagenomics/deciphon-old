#ifndef DCP_DB_H
#define DCP_DB_H

#include "dcp/export.h"
#include <stdint.h>

struct dcp_profile;
struct imm_abc;
struct dcp_db;

DCP_API struct dcp_db *dcp_db_openr(char const *filepath);

DCP_API struct dcp_db *dcp_db_openw(char const *filepath,
                                    struct imm_abc const *abc);

DCP_API int dcp_db_write(struct dcp_db *db, struct dcp_profile const *prof);

DCP_API int dcp_db_close(struct dcp_db *db);

DCP_API struct imm_abc const *dcp_db_abc(struct dcp_db const *db);

#endif
