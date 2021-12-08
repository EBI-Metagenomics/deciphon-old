#include "db_reader.h"

enum rc db_reader_openr(struct db_reader *reader, FILE *fp)
{
    struct db_vtable vtable = {0};
    struct db *db = (struct db *)&reader->db;
    db_init(db, vtable);
    db_openr(db, fp);

    enum rc rc = RC_DONE;
    if ((rc = db_read_magic_number(db))) return rc;
    if ((rc = db_read_prof_type(db))) return rc;
    reader->typeid = (enum db_typeid)db_typeid(db);

    rewind(fp);
    if (reader->typeid == DB_STANDARD)
        return standard_db_openr(&reader->db.std, fp);
    else if (reader->typeid == DB_PROTEIN)
        return protein_db_openr(&reader->db.pro, fp);
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
