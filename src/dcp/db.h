#ifndef DCP_DB_H
#define DCP_DB_H

#include "dcp/export.h"
#include "dcp/meta.h"
#include "dcp/pro/cfg.h"
#include "dcp/profile_types.h"
#include "imm/imm.h"
#include <stdbool.h>
#include <stdio.h>

struct dcp_db;
struct dcp_profile;

struct dcp_db_cfg
{
    enum dcp_profile_typeid prof_typeid;
    unsigned float_bytes;
    struct dcp_pro_cfg pro;
};

static inline struct dcp_db_cfg dcp_db_std(void)
{
    return (struct dcp_db_cfg){DCP_STD_PROFILE, IMM_FLOAT_BYTES,
                               DCP_PRO_CFG_NULL()};
}

static inline struct dcp_db_cfg dcp_db_pro(struct dcp_pro_cfg cfg)
{
    return (struct dcp_db_cfg){DCP_PROTEIN_PROFILE, IMM_FLOAT_BYTES, cfg};
}

DCP_API struct dcp_profile *dcp_db_profile(struct dcp_db *db);

DCP_API struct dcp_db *dcp_db_openr(FILE *restrict fd);

DCP_API struct dcp_db *dcp_db_openw(FILE *restrict fd,
                                    struct imm_abc const *abc,
                                    struct dcp_db_cfg cfg);

DCP_API struct dcp_db_cfg dcp_db_cfg(struct dcp_db const *db);

DCP_API int dcp_db_write(struct dcp_db *db, struct dcp_profile const *prof);

DCP_API int dcp_db_close(struct dcp_db *db);

DCP_API struct imm_abc const *dcp_db_abc(struct dcp_db const *db);

DCP_API unsigned dcp_db_nprofiles(struct dcp_db const *db);

DCP_API struct dcp_meta dcp_db_meta(struct dcp_db const *db, unsigned idx);

DCP_API int dcp_db_read(struct dcp_db *db, struct dcp_profile *prof);

DCP_API bool dcp_db_end(struct dcp_db const *db);

/* DCP_API int dcp_db_reset(struct dcp_db *db); */

#endif
