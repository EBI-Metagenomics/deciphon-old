#ifndef DB_TMP_H
#define DB_TMP_H

#include "common/rc.h"
#include "xlip.h"
#include <stdint.h>

struct db_tmp
{
    union
    {
        struct lip_io_file cmps[4];
        struct
        {
            struct lip_io_file hdr;
            struct lip_io_file mt;
            struct lip_io_file size;
            struct lip_io_file prof;
        } __attribute__((packed));
    };
};

void db_tmp_setup(struct db_tmp *db);
enum rc db_tmp_init(struct db_tmp *db);
void db_tmp_close(struct db_tmp *db);

#endif
