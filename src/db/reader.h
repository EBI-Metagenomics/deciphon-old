#ifndef DB_READER_H
#define DB_READER_H

#include "core/lite_pack.h"
#include "model/profile.h"
#include "model/profile_typeid.h"

struct db_reader
{
    unsigned nprofiles;
    uint32_t *profile_sizes;
    enum profile_typeid profile_typeid;
    struct lip_file file;
};

enum rc db_reader_open(struct db_reader *db, FILE *fp);
void db_reader_close(struct db_reader *db);

enum rc db_reader_unpack_magic_number(struct db_reader *);
enum rc db_reader_unpack_profile_typeid(struct db_reader *,
                                        enum profile_typeid typeid);
enum rc db_reader_unpack_float_size(struct db_reader *);
enum rc db_reader_unpack_profile_sizes(struct db_reader *db);

#endif
