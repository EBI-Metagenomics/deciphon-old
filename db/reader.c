#include "deciphon/db/reader.h"
#include "deciphon/compiler.h"
#include "deciphon/db/types.h"
#include "deciphon/expect.h"
#include "deciphon/limits.h"
#include "deciphon/logger.h"
#include "deciphon/model/model.h"
#include "imm/imm.h"
#include <stdlib.h>

enum rc db_reader_open(struct db_reader *db, FILE *fp)
{
    db->nprofiles = 0;
    db->profile_sizes = 0;
    db->profile_typeid = PROFILE_NULL;
    lip_file_init(&db->file, fp);
    return RC_OK;
}

void db_reader_close(struct db_reader *db)
{
    if (db->profile_sizes) free(db->profile_sizes);
}

enum rc db_reader_unpack_magic_number(struct db_reader *db)
{
    if (!expect_map_key(&db->file, "magic_number")) return eio("read key");
    unsigned number = 0;
    if (!lip_read_int(&db->file, &number)) eio("read magic number");

    return number != MAGIC_NUMBER ? einval("invalid magic number") : RC_OK;
}

enum rc db_reader_unpack_profile_typeid(struct db_reader *db,
                                        enum profile_typeid typeid)
{
    if (!expect_map_key(&db->file, "profile_typeid")) return eio("read key");
    if (!lip_read_int(&db->file, &db->profile_typeid))
        return eio("read typeid");
    if (db->profile_typeid != typeid) return einval("invalid typeid");

    return RC_OK;
}

enum rc db_reader_unpack_float_size(struct db_reader *db)
{
    if (!expect_map_key(&db->file, "float_size")) return eio("read key");
    unsigned size = 0;
    if (!lip_read_int(&db->file, &size)) return eio("read float size");

    return size != IMM_FLOAT_BYTES ? einval("invalid float size") : RC_OK;
}

static enum rc unpack_header_profile_sizes(struct db_reader *db)
{
    enum lip_1darray_type type = 0;

    if (!lip_read_1darray_size_type(&db->file, &db->nprofiles, &type))
        return eio("read array");
    if (type != LIP_1DARRAY_UINT32) return einval("invalid type");

    db->profile_sizes = malloc(sizeof(*db->profile_sizes) * db->nprofiles);
    if (!db->profile_sizes) return enomem("allocate memory");

    if (!lip_read_1darray_u32_data(&db->file, db->nprofiles, db->profile_sizes))
    {
        free(db->profile_sizes);
        db->profile_sizes = 0;
        return eio("read array");
    }

    return RC_OK;
}

enum rc db_reader_unpack_profile_sizes(struct db_reader *db)
{
    if (!expect_map_key(&db->file, "profile_sizes")) return eio("read key");
    return unpack_header_profile_sizes(db);
}
