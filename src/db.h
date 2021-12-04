#ifndef DB_H
#define DB_H

#include "dcp_cmp.h"
#include "meta.h"
#include "profile.h"
#include "profile_types.h"
#include <sys/types.h>

#define DB_HANDLE_MAX_FILES 64

enum db_mode
{
    DB_OPEN_NULL,
    DB_OPEN_READ,
    DB_OPEN_WRITE,
};

struct db
{
    enum profile_typeid prof_typeid;
    unsigned float_size;
    unsigned npartitions;
    struct
    {
        profile_idx_t size;
        profile_idx_t curr_idx;
    } profiles;
    off_t partition_offset[DB_HANDLE_MAX_FILES + 1];
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
        struct cmp_ctx_s cmp[DB_HANDLE_MAX_FILES];
        enum db_mode mode;
    } file;
};

extern struct db const db_default;

unsigned db_float_size(struct db const *db);
enum profile_typeid db_prof_typeid(struct db const *db);
struct meta db_meta(struct db const *db, unsigned idx);
bool db_end(struct db const *db);

void db_init(struct db *db, enum profile_typeid prof_typeid);

void db_openr(struct db *db, FILE *restrict fp);
void db_set_files(struct db *db, unsigned nfiles, FILE *restrict fp[]);
enum rc db_openw(struct db *db, FILE *restrict fp);
enum rc db_close(struct db *db);
bool db_end(struct db const *db);

enum rc db_read_magic_number(struct db *db);
enum rc db_write_magic_number(struct db *db);

enum rc db_read_prof_type(struct db *db);
enum rc db_write_prof_type(struct db *db);

enum rc db_read_float_size(struct db *db);
enum rc db_write_float_size(struct db *db);

enum rc db_read_nprofiles(struct db *db);
enum rc db_read_metadata(struct db *db);

enum rc db_write_prof_meta(struct db *db, struct profile const *prof);

enum rc db_check_write_prof_ready(struct db const *db,
                                  struct profile const *prof);

struct meta db_meta(struct db const *db, unsigned idx);

enum rc db_current_offset(struct db *db, off_t *offset);

enum rc db_record_first_partition_offset(struct db *db);

enum rc db_rewind(struct db *db);

static inline unsigned db_nprofiles(struct db const *db)
{
    return db->profiles.size;
}

#endif
