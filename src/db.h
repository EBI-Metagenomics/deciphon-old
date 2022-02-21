#ifndef DCP_DB_H
#define DCP_DB_H

#include "db_mt.h"
#include "db_tmp.h"
#include "metadata.h"
#include "profile.h"

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
    struct imm_abc const *(*abc)(struct db const *db);
    enum rc (*write_profile)(struct lip_file *dst, struct profile const *prof);
    unsigned header_size;
};

struct db
{
    struct db_vtable vtable;

    int profile_typeid;
    unsigned float_size;
    unsigned nprofiles;

    // TODO: remove it
    int64_t *profile_offsets;

    struct db_mt mt;

    struct
    {
        struct lip_file lip;
        enum db_mode mode;
    } file;

    struct db_tmp tmp;
};

unsigned db_float_size(struct db const *db);
int db_profile_typeid(struct db const *db);
int db_typeid(struct db const *db);
struct metadata db_metadata(struct db const *db, unsigned idx);

void db_init(struct db *db, struct db_vtable vtable);
struct imm_abc const *db_abc(struct db const *db);

void db_openr(struct db *db, FILE *fp);
enum rc db_openw(struct db *db, FILE *fp);
enum rc db_close(struct db *db);
void db_cleanup(struct db *db);

enum rc db_read_magic_number(struct db *db);
enum rc db_write_magic_number(struct db *db);

enum rc db_read_profile_typeid(struct db *db);
enum rc db_write_profile_typeid(struct db *db);

enum rc db_read_float_size(struct db *db);
enum rc db_write_float_size(struct db *db);

enum rc db_read_profile_sizes(struct db *db);
enum rc db_read_metadata(struct db *db);

enum rc db_write_profile_metadata(struct db *db, struct metadata mt);
enum rc db_write_profile(struct db *db, struct profile const *prof,
                         struct metadata mt);

enum rc db_set_metadata_end(struct db *db);

static inline unsigned db_nprofiles(struct db const *db)
{
    return db->nprofiles;
}

#endif
