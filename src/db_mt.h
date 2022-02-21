#ifndef DCP_DB_MT_H
#define DCP_DB_MT_H

#include "metadata.h"
#include "dcp/rc.h"
#include <stdint.h>

struct db_mt
{
    uint32_t *offset;
    uint8_t *name_length;
    uint32_t size;
    char *data;
};

struct lip_file;

void db_mt_init(struct db_mt *db);
void db_mt_cleanup(struct db_mt *db);
enum rc db_mt_read(struct db_mt *db, unsigned nprofiles,
                   struct lip_file *cmp);
enum rc db_mt_write(struct db_mt *db, struct metadata mt,
                    struct lip_file *dst);
struct metadata db_mt_metadata(struct db_mt const *db, unsigned idx);

#endif
