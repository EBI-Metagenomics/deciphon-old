#ifndef DB_H
#define DB_H

#include "dcp/profile.h"
#include "xcmp.h"

enum db_mode
{
    DB_OPEN_READ,
    DB_OPEN_WRIT,
};

struct dcp_db
{
    enum dcp_prof_typeid prof_typeid;
    unsigned float_bytes;
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
            FILE *fd;
            cmp_ctx_t ctx;
        } file;
    } mt;
    struct
    {
        FILE *fd;
    } dp;
    struct
    {
        FILE *fd;
        enum db_mode mode;
        cmp_ctx_t ctx;
    } file;
};

void db_init(struct dcp_db *db);
void db_openr(struct dcp_db *db, FILE *restrict fd);
enum dcp_rc db_read_magic_number(struct dcp_db *db);
enum dcp_rc db_read_prof_type(struct dcp_db *db);
enum dcp_rc db_read_float_bytes(struct dcp_db *db);

#endif
