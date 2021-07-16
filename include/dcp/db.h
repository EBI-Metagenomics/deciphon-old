#ifndef DCP_DB_H
#define DCP_DB_H

#include "dcp/export.h"
#include "dcp/metadata.h"
#include "dcp/pp.h"
#include "imm/imm.h"
#include <stdbool.h>
#include <stdio.h>

struct dcp_profile;
struct imm_abc;
struct dcp_db;

enum dcp_profile_type
{
    DCP_PROFILE_TYPE_PLAIN = 0,
    DCP_PROFILE_TYPE_PROTEIN = 1,
};

struct dcp_db_cfg
{
    enum dcp_profile_type prof_type;
    unsigned float_bytes;
    struct dcp_pp_cfg pp;
};

DCP_API struct dcp_db *dcp_db_openr(FILE *restrict fd);

DCP_API struct dcp_db *dcp_db_openw(FILE *restrict fd,
                                    struct imm_abc const *abc,
                                    struct dcp_db_cfg cfg);

DCP_API struct dcp_db_cfg dcp_db_cfg(struct dcp_db const *db);

DCP_API int dcp_db_write(struct dcp_db *db, struct dcp_profile const *prof);

DCP_API int dcp_db_close(struct dcp_db *db);

DCP_API struct imm_abc const *dcp_db_abc(struct dcp_db const *db);

DCP_API unsigned dcp_db_nprofiles(struct dcp_db const *db);

DCP_API struct dcp_metadata dcp_db_metadata(struct dcp_db const *db,
                                            unsigned idx);

DCP_API int dcp_db_read(struct dcp_db *db, struct dcp_profile *prof);

DCP_API bool dcp_db_end(struct dcp_db const *db);

/* DCP_API int dcp_db_reset(struct dcp_db *db); */

#endif
