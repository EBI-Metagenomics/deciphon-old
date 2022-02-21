#ifndef DB_TMP_H
#define DB_TMP_H

#include "dcp/rc.h"
#include "lite_pack.h"
#include <stdint.h>

struct db_tmp
{
    union
    {
        struct lip_file cmps[4];
        struct
        {
            struct lip_file hdr;
            struct lip_file mt;
            struct lip_file size;
            struct lip_file prof;
        } __attribute__((packed));
    };
};

void db_tmp_setup(struct db_tmp *db);
enum rc db_tmp_init(struct db_tmp *db);
void db_tmp_close(struct db_tmp *db);

#endif
