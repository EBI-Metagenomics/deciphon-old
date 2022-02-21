#ifndef DCP_DB_READER_H
#define DCP_DB_READER_H

#include "dcp/rc.h"
#include "db_types.h"
#include "protein_db.h"
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

struct lip_file;

enum rc db_reader_open(struct db_reader *reader, struct lip_file *io);
struct db *db_reader_db(struct db_reader *reader);
enum rc db_reader_close(struct db_reader *reader);

#endif
