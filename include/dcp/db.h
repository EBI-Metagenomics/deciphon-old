#ifndef DCP_DB_H
#define DCP_DB_H

#include "dcp/export.h"
#include "dcp/meta.h"
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
    imm_float epsilon;
    enum dcp_entry_distr edistr;
    struct imm_nuclt const *nuclt;
    struct imm_amino const *amino;
};

static inline struct dcp_db_cfg dcp_db_std(void)
{
    return (struct dcp_db_cfg){DCP_STD_PROFILE,
                               IMM_FLOAT_BYTES,
                               IMM_LPROB_NAN,
                               DCP_ENTRY_DISTR_NULL,
                               NULL,
                               NULL};
}

static inline struct dcp_db_cfg dcp_db_pro(imm_float epsilon,
                                           enum dcp_entry_distr edistr,
                                           struct imm_nuclt const *nuclt,
                                           struct imm_amino const *amino)
{
    return (struct dcp_db_cfg){
        DCP_PROTEIN_PROFILE, IMM_FLOAT_BYTES, epsilon, edistr, nuclt, amino};
}

DCP_API struct dcp_profile *dcp_db_profile(struct dcp_db *db);

DCP_API struct dcp_db *dcp_db_openr(FILE *restrict fd);

DCP_API struct dcp_db *dcp_db_openw(FILE *restrict fd,
                                    struct imm_abc const *abc,
                                    struct dcp_db_cfg cfg);

DCP_API struct dcp_db_cfg const *dcp_db_cfg(struct dcp_db const *db);

DCP_API int dcp_db_write(struct dcp_db *db, struct dcp_profile const *prof);

DCP_API int dcp_db_close(struct dcp_db *db);

DCP_API struct imm_abc const *dcp_db_abc(struct dcp_db const *db);

DCP_API unsigned dcp_db_nprofiles(struct dcp_db const *db);

DCP_API struct dcp_meta dcp_db_meta(struct dcp_db const *db, unsigned idx);

DCP_API int dcp_db_read(struct dcp_db *db, struct dcp_profile *prof);

DCP_API bool dcp_db_end(struct dcp_db const *db);

/* DCP_API int dcp_db_reset(struct dcp_db *db); */

#endif
