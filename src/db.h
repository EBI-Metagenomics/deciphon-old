#ifndef DB_H
#define DB_H

#include "dcp_cmp.h"
#include "meta.h"
#include "profile.h"
#include "profile_types.h"
#include <sys/types.h>

#define DCP_DB_HANDLE_MAX_FILES 64

enum dcp_db_mode
{
    DB_OPEN_NULL,
    DB_OPEN_READ,
    DB_OPEN_WRITE,
};

struct dcp_db
{
    enum profile_typeid prof_typeid;
    unsigned float_size;
    unsigned npartitions;
    struct
    {
        dcp_profile_idx_t size;
        dcp_profile_idx_t curr_idx;
    } profiles;
    off_t partition_offset[DCP_DB_HANDLE_MAX_FILES + 1];
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

unsigned dcp_db_float_size(struct dcp_db const *db);
enum profile_typeid dcp_db_prof_typeid(struct dcp_db const *db);
unsigned dcp_db_nprofiles(struct dcp_db const *db);
struct meta dcp_db_meta(struct dcp_db const *db, unsigned idx);
bool dcp_db_end(struct dcp_db const *db);

void db_init(struct dcp_db *db, enum profile_typeid prof_typeid);

void db_openr(struct dcp_db *db, FILE *restrict fp);
void db_set_files(struct dcp_db *db, unsigned nfiles, FILE *restrict fp[]);
enum rc db_openw(struct dcp_db *db, FILE *restrict fp);
enum rc db_close(struct dcp_db *db);
bool db_end(struct dcp_db const *db);

enum rc db_read_magic_number(struct dcp_db *db);
enum rc db_write_magic_number(struct dcp_db *db);

enum rc db_read_prof_type(struct dcp_db *db);
enum rc db_write_prof_type(struct dcp_db *db);

enum rc db_read_float_size(struct dcp_db *db);
enum rc db_write_float_size(struct dcp_db *db);

enum rc db_read_nprofiles(struct dcp_db *db);
enum rc db_read_metadata(struct dcp_db *db);

enum rc db_write_prof_meta(struct dcp_db *db, struct profile const *prof);

enum rc db_check_write_prof_ready(struct dcp_db const *db,
                                  struct profile const *prof);

struct meta db_meta(struct dcp_db const *db, unsigned idx);

enum rc db_current_offset(struct dcp_db *db, off_t *offset);

enum rc db_record_first_partition_offset(struct dcp_db *db);

enum rc db_rewind(struct dcp_db *db);

static inline unsigned db_nprofiles(struct dcp_db const *db)
{
    return db->profiles.size;
}

#endif
