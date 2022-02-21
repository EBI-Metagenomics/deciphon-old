#include "db_reader.h"
#include "profile_types.h"
#include "xlip.h"

enum rc db_reader_open(struct db_reader *reader, struct lip_file *io)
{
    struct db_vtable vtable = {0};
    struct db *db = (struct db *)&reader->db;
    /* TODO: implement a db_file_typeid instead */
    db_init(db, vtable);
    db_openr(db, io);

    if (xlip_expect_key(&db->file.cmp, "header")) return RC_EPARSE;
    unsigned size = 0;
    lip_read_map_size(&db->file.cmp, &size);

    enum rc rc = RC_DONE;
    if ((rc = db_read_magic_number(db))) return rc;
    if ((rc = db_read_profile_typeid(db))) return rc;

    if (db->profile_typeid == PROFILE_STANDARD)
        reader->typeid = DB_STANDARD;
    else if (db->profile_typeid == PROFILE_PROTEIN)
        reader->typeid = DB_PROTEIN;
    else
        assert(false);

    rewind(io->fp);
    if (reader->typeid == DB_STANDARD)
        return standard_db_openr(&reader->db.std, io);
    else if (reader->typeid == DB_PROTEIN)
        return protein_db_openr(&reader->db.pro, io);
    else
        assert(false);

    return rc;
}

struct db *db_reader_db(struct db_reader *reader)
{
    return (struct db *)&reader->db;
}

enum rc db_reader_close(struct db_reader *reader)
{
    return db_close(db_reader_db(reader));
}
