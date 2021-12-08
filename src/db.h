#ifndef DB_H
#define DB_H

#include "cmp/cmp.h"
#include "dcp_limits.h"
#include "metadata.h"
#include "profile.h"
#include <sys/types.h>

enum db_mode
{
    DB_OPEN_NULL,
    DB_OPEN_READ,
    DB_OPEN_WRITE,
};

struct db;

struct db_vtable
{
    int typeid;
    enum rc (*close)(struct db *db);
};

struct db
{
    struct db_vtable vtable;

    unsigned float_size;
    unsigned npartitions;
    struct
    {
        uint32_t size;
        uint32_t curr_idx;
    } profiles;
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
        struct cmp_ctx_s cmp[DCP_MAX_OPEN_DB_FILES];
        enum db_mode mode;
    } file;
};

extern struct db const db_default;

unsigned db_float_size(struct db const *db);
enum profile_typeid db_prof_typeid(struct db const *db);
struct metadata db_meta(struct db const *db, unsigned idx);
bool db_end(struct db const *db);

void db_init(struct db *db, struct db_vtable vtable);

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

struct metadata db_meta(struct db const *db, unsigned idx);

enum rc db_current_offset(struct db *db, off_t *offset);

enum rc db_record_first_partition_offset(struct db *db);

enum rc db_rewind(struct db *db);

static inline unsigned db_nprofiles(struct db const *db)
{
    return db->profiles.size;
}

#endif
