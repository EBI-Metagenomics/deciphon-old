#ifndef DCP_DB_H
#define DCP_DB_H

#include "dcp/export.h"
#include "dcp/meta.h"
#include "dcp/prof_types.h"

struct dcp_db;

DCP_API unsigned dcp_db_float_size(struct dcp_db const *db);
DCP_API enum dcp_prof_typeid dcp_db_prof_typeid(struct dcp_db const *db);
DCP_API unsigned dcp_db_nprofiles(struct dcp_db const *db);
DCP_API struct dcp_meta dcp_db_meta(struct dcp_db const *db, unsigned idx);
DCP_API bool dcp_db_end(struct dcp_db const *db);

#endif
