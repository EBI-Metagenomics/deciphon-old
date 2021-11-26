#ifndef DCP_DB_H
#define DCP_DB_H

#include "dcp/cmp.h"
#include "dcp/export.h"
#include "dcp/meta.h"
#include "dcp/prof_types.h"

#define DCP_DB_HANDLE_MAX_FILES 64

enum dcp_db_mode
{
    DB_OPEN_NULL,
    DB_OPEN_READ,
    DB_OPEN_WRITE,
};

struct dcp_db
{
    enum dcp_prof_typeid prof_typeid;
    unsigned float_size;
    unsigned npartitions;
    struct
    {
        dcp_profile_idx_t size;
        dcp_profile_idx_t curr_idx;
    } profiles;
    off_t partition_offset[DCP_DB_HANDLE_MAX_FILES];
    struct
    {
        uint32_t *offset;
        uint8_t *name_length;
        uint32_t size;
        char *data;
        struct
        {
            struct cmp_ctx_s cmp;
        } file;
    } mt;
    struct
    {
        struct cmp_ctx_s cmp;
    } dp;
    struct
    {
        struct cmp_ctx_s cmp[DCP_DB_HANDLE_MAX_FILES];
        enum dcp_db_mode mode;
    } file;
};

extern struct dcp_db const dcp_db_default;

DCP_API unsigned dcp_db_float_size(struct dcp_db const *db);
DCP_API enum dcp_prof_typeid dcp_db_prof_typeid(struct dcp_db const *db);
DCP_API unsigned dcp_db_nprofiles(struct dcp_db const *db);
DCP_API struct dcp_meta dcp_db_meta(struct dcp_db const *db, unsigned idx);
DCP_API bool dcp_db_end(struct dcp_db const *db);

#endif
