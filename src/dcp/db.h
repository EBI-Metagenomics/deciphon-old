#ifndef DCP_DB_H
#define DCP_DB_H

#include "dcp/cmp.h"
#include "dcp/export.h"
#include "dcp/meta.h"
#include "dcp/prof_types.h"

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
    struct
    {
        dcp_profile_idx_t size;
        dcp_profile_idx_t curr_idx;
    } profiles;
    struct
    {
        uint32_t *offset;
        uint8_t *name_length;
        uint32_t size;
        char *data;
        struct
        {
            struct dcp_cmp cmp;
        } file;
    } mt;
    struct
    {
        struct dcp_cmp cmp;
    } dp;
    struct
    {
        struct dcp_cmp cmp;
        enum dcp_db_mode mode;
    } file;
};

DCP_API unsigned dcp_db_float_size(struct dcp_db const *db);
DCP_API enum dcp_prof_typeid dcp_db_prof_typeid(struct dcp_db const *db);
DCP_API unsigned dcp_db_nprofiles(struct dcp_db const *db);
DCP_API struct dcp_meta dcp_db_meta(struct dcp_db const *db, unsigned idx);
DCP_API bool dcp_db_end(struct dcp_db const *db);

#endif
