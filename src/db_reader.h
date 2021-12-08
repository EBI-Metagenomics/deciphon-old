#ifndef DB_READER_H
#define DB_READER_H

#include "db_types.h"
#include "protein_db.h"
#include "rc.h"
#include "standard_db.h"
#include <stdio.h>

struct db_reader
{
    enum db_typeid typeid;
    union
    {
        struct standard_db std;
        struct protein_db pro;
    } db;
};

enum rc db_reader_openr(struct db_reader *reader, FILE *fp);
struct db *db_reader_db(struct db_reader *reader);
enum rc db_reader_close(struct db_reader *reader);

#endif
