#ifndef DECIPHON_DB_WRITER_H
#define DECIPHON_DB_WRITER_H

#include "deciphon/lite_pack.h"
#include "deciphon/model/profile.h"

typedef enum rc (*pack_profile_func_t)(struct lip_file *file, void const *arg);
typedef enum rc (*pack_header_item_func_t)(struct lip_file *file,
                                           void const *arg);

struct db_writer
{
    unsigned nprofiles;
    unsigned header_size;
    struct lip_file file;
    struct
    {
        struct lip_file header;
        struct lip_file profile_sizes;
        struct lip_file profiles;
    } tmp;
};

enum rc db_writer_open(struct db_writer *db, FILE *fp);
enum rc db_writer_close(struct db_writer *db, bool successfully);

enum rc db_writer_pack_magic_number(struct db_writer *);
enum rc db_writer_pack_profile_typeid(struct db_writer *, int profile_typeid);
enum rc db_writer_pack_float_size(struct db_writer *);
enum rc db_writer_pack_profile(struct db_writer *db,
                               pack_profile_func_t pack_profile,
                               void const *arg);
enum rc db_writer_pack_header_item(struct db_writer *db,
                                   pack_header_item_func_t pack_header_item,
                                   void const *arg);

#endif
